#if !defined(COMMON_H)
#define COMMON_H

#include <cstddef>
#include <cstdint>
#include <sstream>
#include "AL/al.h"
#include "AL/alc.h"

// TODO remove this. I used this for debugging
typedef std::uint8_t buf_type;

/**
 * @brief A struct to hold decoded audio information from read_audio_info_buffer().
 * FFMPEG documentation https://ffmpeg.org/doxygen/trunk/index.html
 */
struct audio_info{
    std::basic_stringstream<buf_type>* buffer;
    int sample_rate;
    const char* format;
    // Number of bytes in buffer
    size_t buffer_size;
};

/**
 * @brief Create the OpenAL context, and bind to the default audio device.
 * 
 * @return ALCcontext* created openal context
 */
ALCcontext* init_openal();
/**
 * @brief Load the provided audio_info decoded from read_audio_info_buffer()
 * 
 * @param info created form read_audio_info_buffe()
 * @return ALuint the ID to the buffer for play_sound()
 */
ALuint load_sound(audio_info info);

/**
 * @brief Play a sound into the opened OpenAL context init_openal()
 * 
 * @param buffer buffer from load_sound()
 */
void play_sound(ALuint buffer);
/**
 * @brief Destroy openal buffer
 * 
 * @param buffer buffer from load_sound()
 */
void delete_openal_source(ALuint buffer);
/**
 * @brief Close the opened openal context
 */
void close_openal();

/**
 * @brief Creates an audio_info object by decoding audio
 * in the filename by using FFMPEG. This will output PCM audio data.
 * This can only be used on audio files, * and assumes the first
 * stream is the audio stream.
 * 
 * @param filename The file to read the audio from 
 * @return audio_info* 
 */
audio_info* read_audio_into_buffer(const char* filename);


#endif // COMMON_H
