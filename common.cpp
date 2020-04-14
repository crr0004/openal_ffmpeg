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

void format_av_error(int ret){
    // Only want to trigger this on unhandable errors
	if(ret != 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
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

std::basic_stringstream<std::uint16_t>* decode(
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

    int i, ch = 0;
    int ret, data_size = 0;

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *outFrame = av_frame_alloc();

    /* 
    Grab our first packet from the file
    */
    ret = av_read_frame(form_ctx, pkt);
    format_av_error(ret);

    ret = avcodec_send_packet(dec_ctx, pkt);
    format_av_error(ret);

    /* read all the output frames (in general there may be any number of them */
    std::basic_stringstream<std::uint16_t> *stream = new std::basic_stringstream<std::uint16_t>();
    bool is_eof = false;
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
        format_av_error(ret);
        if(is_eof){
            // TODO Again lift this break out of the loop so the file reading is done properly
            break;
        }

        // Copy the frame properties as that is what SWR wants
        // https://ffmpeg.org/doxygen/trunk/group__lswr.html#gac482028c01d95580106183aa84b0930c
        outFrame->channel_layout = frame->channel_layout;
        outFrame->sample_rate = frame->sample_rate;
        outFrame->format = frame->format;

        // Convert our frame to what we want through SWR
        ret = swr_convert_frame(swr, outFrame, frame);
        format_av_error(ret);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            format_av_error(ret);
        }
        // ret = avresample_convert_frame(avr, outFrame, frame);

        // data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        // if (data_size < 1) {
        //     format_av_error(nullptr, "Could not get size of sample");
        // }

        // We only care about the first channel because the data is converted to packed
        size_t size = outFrame->linesize[0]/sizeof(std::uint16_t);
        stream->write((std::uint16_t*)frame->data[0], size);
        // for (i = 0; i < outFrame->nb_samples; i++){
        //         // fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
        //         // stream->write(frame->data[ch], data_size);
        //         // std::cout << stream->tellg();
        //         // std::cout << "C" << 0 << ": " << size << std::endl;
        //         for(int j = 0; j < size; j++){
        //             // std::cout << "" << ((std::uint16_t)outFrame->data[0][j]) << ", ";
        //         } 
        //         // std::cout << std::endl;
        // }
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
		AVDictionary *options;

		//Find the decoder for the audio stream
		pCodec = avcodec_find_decoder(pCodecContext->codec_id);

		printf("TEST codec name = %s fullName = %s\n",pCodec->name, pCodec->long_name);
	}  

}

audio_info* read_audio_into_buffer(const char* filename) {

    AVFormatContext *avFormatContext = NULL;
    // Return code for calls
    int ret = 0;    


	ret = avformat_open_input(&avFormatContext, filename, NULL, NULL);
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

    // Set the audio conversion options so it can be pushed into OpenAL
    // OpenAL needs PCM (packed) mono or stero, 8 or 16 bit
    // we just sample to stero for simplicity
    // See https://ffmpeg.org/doxygen/trunk/group__lswr.html for all the options
    SwrContext *swr = swr_alloc_set_opts(
        NULL,
        AV_CH_LAYOUT_STEREO,
        AV_SAMPLE_FMT_S16,
        codec_context->sample_rate,
        codec_context->channel_layout,
        (AVSampleFormat)codec_context->format,
        codec_context->sample_rate,
        0,
        NULL 
    );
    format_av_error(swr, "Something went wrong with allocating resample context");

    
    // Heap create this so we can return it out
    std::basic_stringstream<std::uint16_t> *stream = decode(c, avFormatContext, swr);

    // Create a struct so we can return out some information
    // in a format neutral way
    // We allocate on the heap so it doesn't get deleted when we return
    audio_info *info = new audio_info;
    info->buffer = stream;
    info->sample_rate = 44100;
    info->format = av_get_sample_fmt_name(AV_SAMPLE_FMT_S16);

    avcodec_free_context(&c);
    avformat_free_context(avFormatContext);

    return info;
};