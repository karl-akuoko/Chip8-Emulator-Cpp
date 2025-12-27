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

void CPU::decode_and_execute(uint16_t instruction) {
    // Extract common variables from the opcode
    uint8_t kk = static_cast<uint8_t>(instruction & 0xFF);
    uint16_t nnn = instruction & 0xFFF;
    uint8_t n = static_cast<uint8_t>(instruction & 0xF);
    uint8_t x = static_cast<uint8_t>(instruction & 0xF00) >> 8;
    uint8_t y = static_cast<uint8_t>(instruction & 0xF0) >> 4;
    
    // first nibble determines instruction vategorm
    uint8_t first_nibble = static_cast<uint8_t>(instruction & 0xF000) >> 12;
    
    switch (first_nibble) {
        case 0x0:
            // 00E0: Clear the display
            if (instruction == 0x00E0) {
                display.fill(0);
            // 00EE: Return from subroutine
            } else if (instruction == 0x00EE) {
                --sp;
                pc = stack[sp];
            }
            break;
        
        case 0x1: // Jump to location nnn
            pc = nnn;
            break;
        
        case 0x2: // Call subroutine at nnn
            stack[sp] = pc;
            ++sp;
            pc = nnn;
            break;
        
        case 0x3: // Skip next instruction if Vx = kk
            if (registers[x] == kk) {
                pc += 2;
            }
            break;
        
        case 0x4: // Skip next instruction if Vx != kk
            if (registers[x] != kk) {
                pc += 2;
            }
            break;
        
        case 0x5: // Skip next instruction if Vx = Vy
            if (registers[x] == registers[y]) {
                pc += 2;
            }
            break;
        
        case 0x6: // Set Vx = kk
            registers[x] = kk;
            break;
        
        case 0x7: // Set Vx = Vx + kk
            registers[x] = registers[x] + kk;
            break;
        
        case 0x8:
            switch(n) {
                case 0x0: // Set Vx = Vy
                    registers[x] = registers[y];
                    break;
                case 0x1: // Set Vx = Vx OR Vy
                    registers[x] = registers[x] | registers[y];
                    break;
                case 0x2: // Set Vx = Vx AND Vy
                    registers[x] = registers[x] & registers[y];
                    break;
                case 0x3: // Set Vx = Vx XOR Vy
                    registers[x] = registers[x] ^ registers[y];
                    break;
                case 0x4: // Set Vx = Vx + Vy, set VF = carry
                    uint16_t sum = static_cast<uint16_t>(registers[x] 
                        + registers[y]);
                    registers[0xF] = (sum > 255) ? 1 : 0;
                    registers[x] = sum & 0xFF; 
                    break;
                case 0x5: 

            }
            break;
        default:
            break;
    }
}