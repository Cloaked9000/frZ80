#include <iostream>
#include <fstream>
#include <sstream>
#include "Emulator.h"

void io_handler(Emulator::PortState state, uint8_t *data, uint16_t size)
{
    if(state == Emulator::PortState::Write)
    {
        std::cout.write((char*)data, size);
    }
    else
    {
       // std::cin.read((char*)data, size);
        std::cin >> data;
    }
}

int main()
{
    std::ifstream input("out.bin", std::ios::in | std::ios::binary);
    input.seekg(0, input.end);
    auto file_size = input.tellg();
    input.seekg(0, input.beg);
    std::vector<uint8_t> file_data(file_size, '\0');
    input.read(reinterpret_cast<char *>(&file_data[0]), file_size);


    std::stringstream log_stream;

    Emulator emulator;
    emulator.bind_port(0, io_handler);
    emulator.emulate(file_data, log_stream);
    std::cout << "\n--- Decoded Input --- \n" << log_stream.str() << std::endl;
    return 0;
}