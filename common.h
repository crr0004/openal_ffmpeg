#if !defined(COMMON_H)
#define COMMON_H

#include <cstddef>
#include <cstdint>
#include <sstream>

struct audio_info{
    std::basic_stringstream<std::uint16_t>* buffer;
    int sample_rate;
    const char* format;
};

audio_info* read_audio_into_buffer(
    const char*
    );


#endif // COMMON_H
