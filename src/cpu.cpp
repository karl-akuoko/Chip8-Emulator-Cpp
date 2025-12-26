#include "../include/cpu.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>

const unsigned int FONTSET_START_ADDRESS = 0x50;

uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

CPU::CPU()
{
    // Set Program Counter to the start of most programs
    pc = 0x200;

    // Initialize registers and memory to zero
    index_register = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    registers.fill(0);
    memory.fill(0);
    stack.fill(0);
    display.fill(0);

    // Load fontset after clearing memory
    for (unsigned int i = 0; i < 80; i++) {
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}

void CPU::loadROM(const std::string& filename) {
    // Open the file in binary mode and move to the end to find the size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open()) {
        std::streamsize size = file.tellg();

        // Avaliable space calculation
        const size_t MAX_ROM_SIZE = memory.size() - 0x200;
        
        // ROM can not overfill memory
        if (size > MAX_ROM_SIZE) {
            std::cerr << "Error: ROM file is too large (" << size <<
             " bytes)." << std::endl;
        }

        // Creates a unique_ptr to manage the character array 
        auto buffer = std::make_unique<char[]>(size);

        // Read file into buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer.get(), size);
        file.close();

        // Load the buffer into the CHIP-8 memory starting at 0x200
        for (size_t i = 0; i < size; ++i) {
            memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
        }

        std::cout << "Successfully loaded ROM: " << filename << " (" <<
            size << "bytes)" << std::endl;

    } else {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
    }

}

void CPU::cycle() {
    // FETCH: Combines two bytes into 16-bit instruction
    uint16_t instruction = (memory[pc] << 8) | memory[pc + 1];

    // Increment PC
    pc += 2;

    // DECODE and EXECUTE
    decode_and_execute(instruction);
}