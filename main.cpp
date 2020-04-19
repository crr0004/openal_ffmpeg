#include "common.h"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>

int main(int argc, char const *argv[])
{
    audio_info *stream = read_audio_into_buffer(
            "airtoneresonance1.mp3"
        );

    init_openal();
    ALuint sound_buffer = load_sound(*stream);
    play_sound(sound_buffer);
    alDeleteBuffers(1, &sound_buffer);
    close_openal();

    return 0;
}
