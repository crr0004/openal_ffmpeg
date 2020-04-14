#include "common.h"
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>

int main(int argc, char const *argv[])
{
    std::uint8_t *out;
    std::size_t outSize;

    auto stream = read_audio_into_buffer(
            "airtoneresonance1.mp3"
        );
    std::cout << stream->buffer->tellp() << std::endl;
    // std::basic_stringstream<uint8_t> stream;
    // stream.put(42);
    // stream.put(10);
    // stream.put(4);
    // stream.flush();

    // stream.seekg(0);
    // do{
    //     uint8_t out;
    //     stream.read(&out, 1);
    //     std::cout << out << ", ";
    // }while(stream.eof());

    return 0;
}
