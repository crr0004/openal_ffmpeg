#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>

extern "C"{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h> 
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

AVSampleFormat OUT_FORMAT = AV_SAMPLE_FMT_S16;
ALenum AL_OUT_FORMAT = AL_FORMAT_STEREO16;

void format_av_error(int ret){
    // Only want to trigger this on unhandable errors
	if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
		char errbuff[1028];
		av_strerror(ret, errbuff, 1028);
		fprintf(stderr, "Error message (%d): %s\n", ret, errbuff);
        exit(ret);
	}
}
void format_av_error(void* pointer, const char* error_msg){
    if(pointer == nullptr){
        fprintf(stderr, "%s\n", error_msg);
        exit(-1);
    }
}

void dump_av_opt(void* ctx){

    AVSampleFormat fmt;
    av_opt_get_sample_fmt(ctx, "out_sample_fmt", 0, &fmt);
    const char * out_fmt_name = av_get_sample_fmt_name(fmt);
    printf("Out format %s\n", out_fmt_name);

    av_opt_get_sample_fmt(ctx, "in_sample_fmt", 0, &fmt);
    const char * in_fmt_name = av_get_sample_fmt_name(fmt);
    printf("in format %s\n", in_fmt_name);
}

std::basic_stringstream<buf_type>* decode(
        AVCodecContext *dec_ctx,
        AVFormatContext *form_ctx,
        SwrContext *swr) {

    /*
    In this context of reading a file, decoding it, resampling it, and reading it memory
    the following words have the following meanings.

    Packet: a set of data read from a file/IO stream
    Frame: Encoded/Decoded data from the library
    Format: How the data is stored in the file
    Codec: The kind of compression/decompression used on the file
    Stream: One kind of data set in the file (audio, video, subtitles)

    */

    int ret = 0;

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    /* 
        Grab our first packet from the file
    */
    ret = av_read_frame(form_ctx, pkt);
    format_av_error(ret);

    ret = avcodec_send_packet(dec_ctx, pkt);
    format_av_error(ret);

    // Pointers for our data buffers for the converter to write into
    uint8_t *outData;
    int lineSize;

    // Keeping track if the amount of frames decoded changes
    int last_nb_samples = 0;

    /* read all the output frames (in general there may be any number of them */
    std::basic_stringstream<buf_type> *stream = new std::basic_stringstream<buf_type>();
    bool is_eof = false;

    ret = 0; // Clear the error number so it's not confused
    while (ret >= 0 && !is_eof) {
        do{
            if(ret == AVERROR(EAGAIN)){
                // We need more data from the file to decode
                // So grab another packet and then read it into a frame
                ret = av_read_frame(form_ctx, pkt);
                format_av_error(ret);
                if(ret == AVERROR_EOF){
                    // TODO
                    // This should probably be fixed.
                    // This loop needs to be elevated out
                    is_eof = true;
                    break;
                }
                ret = avcodec_send_packet(dec_ctx, pkt);
                format_av_error(ret);
            }
            // This spits out EAGAIN when it needs another frame from the packet stream
            ret = avcodec_receive_frame(dec_ctx, frame);
        }while(ret == AVERROR(EAGAIN));

        if(is_eof){
            // TODO Again lift this break out of the loop so the file reading is done properly
            break;
        }
        
        // If the frame amount has changed, we need to re-allocate our buffer
        if(frame->nb_samples != last_nb_samples){
            ret = av_samples_alloc(
                &outData,
                &lineSize,
                // As we are only doing alignment conversion keep we can cheat here
                // by using the de-coded frame settings. Normally you have to figure this out
                // by using the format changes. E.G Upsampling sample rate
                frame->channels, 
                frame->nb_samples,
                OUT_FORMAT,
                0);
                last_nb_samples = frame->nb_samples;
        }

        // Convert our frame to what we want through SWR
        const uint8_t** inBuf = (const uint8_t**)(frame->extended_data);
        format_av_error(ret);
        // Convert the audio to correct PCM format
        ret = swr_convert(
            swr,
            &outData,
            // Again cheating the samples because we keep the same sample rate when converting
            frame->nb_samples,
            inBuf,
            frame->nb_samples
        );
        format_av_error(ret);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            format_av_error(ret);
        }

        // We only care about the first channel because the data is converted to packed
        size_t size = lineSize;
        stream->write(outData, size);
    }
    return stream;
}

void dump_format_context(AVFormatContext *avFormatContext){
	printf("num of streams = %u\n", *(&avFormatContext->nb_streams));
	printf("filename = %s\n", *(&avFormatContext->url));
	printf("start time = %ld \n", *(&avFormatContext->start_time));
	printf("duration = %ld \n", *(&avFormatContext->duration));
	printf("bit rate = %ld\n", *(&avFormatContext->bit_rate));
	printf("audio codec id = %d \n\n\n", *(&avFormatContext->audio_codec_id));

	AVCodecParameters * pCodecContext;
	for(int i = 0; i < avFormatContext->nb_streams; i++)
	{
		printf("Stream %d\tcodec: %d\n", i, avFormatContext->streams[i]->codecpar->codec_id);
		pCodecContext = avFormatContext->streams[i]->codecpar;
		AVCodec *pCodec;

		//Find the decoder for the audio stream
		pCodec = avcodec_find_decoder(pCodecContext->codec_id);

		printf("TEST codec name = %s fullName = %s\n",pCodec->name, pCodec->long_name);
	}  

}

audio_info* read_audio_into_buffer(const char* filename) {

    AVFormatContext *avFormatContext = NULL;
    // Return code for calls
    int ret = 0;


    // Auto figure out the codec and audio settings
	ret = avformat_open_input(&avFormatContext, filename, NULL, NULL);
	format_av_error(ret);

    // Ensure the codecs are actually decoded by reading the headers
    ret = avformat_find_stream_info(avFormatContext, NULL);
	format_av_error(ret);

	dump_format_context(avFormatContext);

    // We assume the first stream is the audio stream
    // This can be replaced with a call to find_best_stream
	AVCodecParameters *codec_context = avFormatContext->streams[0]->codecpar;
    format_av_error(codec_context, "Could not find stream 0");
    
    // Grab the codec ID for the stream
	AVCodec *codec = avcodec_find_decoder(codec_context->codec_id);
    format_av_error(codec, "Could not find codec");

    AVCodecParserContext* parser = av_parser_init(codec->id);
    format_av_error(parser, "Could not create parser");

    AVCodecContext *c = avcodec_alloc_context3(codec);
    format_av_error(c, "Could not alloc codec");

    ret = avcodec_open2(c, codec, NULL);
    format_av_error(ret);

    /*
        Set the audio conversion options so it can be pushed into OpenAL
        OpenAL needs PCM (packed) mono or stero, 8 or 16 bit
        we just sample to stero for simplicity and keep all the incoming audio settings.
        If you wish to change the settings for sample rate changes, or layout changes,
        then changes to the decode method need to account for this
        See https://ffmpeg.org/doxygen/trunk/group__lswr.html for all the options
    */
    SwrContext *swr = swr_alloc_set_opts(
        NULL,
        AV_CH_LAYOUT_STEREO,
        OUT_FORMAT,
        codec_context->sample_rate,
        codec_context->channel_layout,
        (AVSampleFormat)codec_context->format,
        codec_context->sample_rate,
        0,
        NULL 
    );
    swr_init(swr);
    format_av_error(swr, "Something went wrong with allocating resample context");
    dump_av_opt(swr);

    
    // Heap create this so we can return it out
    std::basic_stringstream<buf_type> *stream = decode(c, avFormatContext, swr);
    stream->flush();
    stream->seekg(0);

    // Create a struct so we can return out some information
    // in a format neutral way
    // We allocate on the heap so it doesn't get deleted when we return
    audio_info *info = new audio_info;
    info->buffer = stream;
    info->sample_rate = codec_context->sample_rate;
    info->format = av_get_sample_fmt_name(OUT_FORMAT);
    info->buffer_size = stream->tellp();

    swr_close(swr);
    avcodec_free_context(&c);
    avformat_free_context(avFormatContext);

    return info;
};


// OpenAL Below here
#include "AL/al.h"
#include "AL/alc.h"
#include <assert.h>

/* LoadBuffer loads the named audio file into an OpenAL buffer object, and
 * returns the new buffer ID.
 */
ALuint load_sound(audio_info info) {

	ALenum err;
    ALuint buffer;
    const buf_type *membuf;
    ALsizei num_bytes;

    // Double check we are at the start of the buffer
    info.buffer->seekg(0);

    // For some reason, we need to grab the buffer THEN get the underlying pointer
    auto p = info.buffer->rdbuf()->str();
    membuf = p.c_str();

    /* Buffer the audio data into a new buffer object, then free the data and
     * close the file.
     */
    buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(
            buffer, 
            AL_OUT_FORMAT,
            membuf, 
            info.buffer_size, 
            info.sample_rate
        );

    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if(err != AL_NO_ERROR)
    {
        fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
        if(buffer && alIsBuffer(buffer))
            alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}

ALCcontext* init_openal(){
    const ALCchar *name;
    ALCdevice *device;
    ALCcontext *ctx;

    /* Open and initialize a device */
    device = NULL;
    device = alcOpenDevice(NULL);
    if(!device)
    {
        fprintf(stderr, "Could not open a device!\n");
        return nullptr;
    }

    ctx = alcCreateContext(device, NULL);
    if(ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE)
    {
        if(ctx != NULL)
            alcDestroyContext(ctx);
        alcCloseDevice(device);
        fprintf(stderr, "Could not set a context!\n");
        return nullptr;
    }

    name = NULL;
    if(alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
    if(!name || alcGetError(device) != AL_NO_ERROR)
        name = alcGetString(device, ALC_DEVICE_SPECIFIER);
    printf("Opened \"%s\"\n", name);
    return ctx;
}
void al_nssleep(unsigned long nsec){
#ifndef _MSC_VER
    struct timespec ts, rem;
    ts.tv_sec = (time_t)(nsec / 1000000000ul);
    ts.tv_nsec = (long)(nsec % 1000000000ul);

    while(nanosleep(&ts, &rem) == -1 && errno == EINTR)
        ts = rem;
#endif
}

void play_sound(ALuint buffer){
    ALuint source = 0;
    ALenum state;
    ALfloat offset;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, (ALint)buffer);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Play the sound until it finishes. */
    alSourcePlay(source);
    do {
        al_nssleep(10000000);
        alGetSourcei(source, AL_SOURCE_STATE, &state);

        /* Get the source offset. */
        alGetSourcef(source, AL_SEC_OFFSET, &offset);
        printf("\rOffset: %f  ", offset);
        fflush(stdout);
    } while(alGetError() == AL_NO_ERROR && state == AL_PLAYING);
    printf("\n");
    alDeleteSources(1, &source);
}

void delete_openal_source(ALuint source, ALuint buffer){
    alDeleteBuffers(1, &buffer);
}

void close_openal(){
    ALCdevice *device;
    ALCcontext *ctx;

    ctx = alcGetCurrentContext();
    if(ctx == NULL)
        return;

    device = alcGetContextsDevice(ctx);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}

