# Writeup for OpenAL Spike
## Topics
- OpenAL Brief
- FFMPEG Brief
- Code Locations
- Program Flow

# Code Locations
## FFMPEG Decode
`common.cpp line 1-263`

## OpenAL
`common.cpp line 266-393`

# OpenAL Brief
OpenAL is a specification implemented by libraries, and the one used here is openal-soft. It is a state machine API that connects raw audio data to devices and plays it. As it is a state machine, you don't get a lot of information from it like more traditional programming. Instead, you get IDs out of API calls.

# FFMPEG Brief
FFMEG is a set of libraries that can do a lot with audio and video. It is used here to decode the audio and resample it. We use the libraries avcodec, avformat, avutil, swresample.

We have to use resampling to get the audio in the write format (PCM).

## FFMPEG Glossary
- Packet: a set of data read from a file/IO stream
- Frame: Encoded/Decoded data from the library
- Format: How the data is stored in the file
- Codec: The kind of compression/decompression used on the file
- Stream: One kind of data set in the file (audio, video, subtitles)

# Program Flow
The main program flow is outlined in `main.cpp`. First we need to decode the audio through `read_audio_into_buffer` and then pass it to OpenAL to play.

## Reading audio
### Invariant Assumptions
- File only contains audio
- File contains decodable audio
- First stream in file is audio

### Decode setup
- Lines 193, 197, 204.
    - First we need to open the file and decode the header of the file to figure out what the codec is. 
- `format_av_error` is just a helper function for dealing with errors.
- Lines 208, 211, 214, 217
    - Open all the codecs and allocate the various structs that the API uses. The file streams are opened and held in the structures.
- Lines 228 - 241
    - Allocate the resampling structure and set the options. We only change the format, and keep all the other settings the same as the input.
- Lines 245
    - This is the function for decoding the audio and reading into a STL buffer.
    - I use STL buffer because it handles all the expansion of the buffer, as predicting the size can be difficult.
- Lines 252 - 262
    - This is where we setup the return structure, and then deallocate all the API structures. If you were to integrate this elsewhere, you could keep the API structures allocated until the program stops.

### Decode Function
This function reads the file, decodes it, resamples it, and then stores in the buffer.
- Lines 70-71
    - Allocates the decoding packet and frame. 
- Line 75 and 78
    - This is where we read the first set of data (packet) from the file and send it to the decoder to read later.
- Line 89
    - Create a buffer to hold the data. Note `buf_type` is a typedef and needs to removed as it was used for debugging
- Line 93-157
    - This is where we read the decoded data, resample it, and store it in the buffer, and if we're out of data to read from the decoder, push the next packet into the decoder.
- Line 94-112 is where we check for the next frame to decode, and keep trying until we get the data, or it errors out. EAGAIN is an error code to send another packet into the decoder. I'm sure there is a better way to do this. The `is_eof` is a check for breaking out of the loop when we have reached the end of the file.
- Line 120-132
    - This is for allocating a buffer for storing the resampled data. It uses the sample rate and number of samples from the decoded frame to determine how much space we need. Note, this only works using the decoded frame because we kept the resample settings the same as the audio file at the start.
    - For a correct allocation setup, something like this would be required https://ffmpeg.org/doxygen/trunk/resampling__audio_8c_source.html#l00081
- Line 138-146
    - The resample actually occurs here. Again using the decoded frame as the sample setup, but only possible because of the resampling setup
- Line 156 is where we write the decoded from the resampling into our buffer

### OpenAL Playing
Most of the OpenAL code comes from the openal-soft example in alplay.c
- Line 313-345
    - We create the OpenAL context and bind to the default audio device
- Line 274-310
    - Takes the audio structure we created earlier to load the raw audio into an OpenAL buffer, returning the ID we get.
- Line 353-374
    - Take the buffer ID we got before, and load that into a source buffer and play that.
- Line 362-371
    - This sleeps for a second and gets the offset for how long the sound has been playing