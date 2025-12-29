#include "../include/cpu.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>
#include <random>

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
    keypad.fill(0);
    buzzer_playing = false; 
    
    // ready for first sprite
    vblank_ready = true;

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

void CPU::updateTimers() {
    if (delay_timer > 0) {
        --delay_timer;
    }

    if (sound_timer > 0) {
        buzzer_playing = true;
        --sound_timer;
    } else {
        buzzer_playing = false; 
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
    uint8_t kk = instruction & 0xFF;
    uint16_t nnn = instruction & 0xFFF;
    uint8_t n = instruction & 0xF;
    uint8_t x = (instruction & 0xF00) >> 8;
    uint8_t y = (instruction & 0xF0) >> 4;
    
    // first nibble determines instruction category
    uint8_t first_nibble = (instruction & 0xF000) >> 12;
    
    switch (first_nibble) {
        case 0x0:
            // Clear the display
            if (instruction == 0x00E0) {
                display.fill(0);
            // Return from subroutine
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
                    registers[0xF] = 0;
                    break;
                case 0x2: // Set Vx = Vx AND Vy
                    registers[x] = registers[x] & registers[y];
                    registers[0xF] = 0;
                    break;
                case 0x3: // Set Vx = Vx XOR Vy
                    registers[x] = registers[x] ^ registers[y];
                    registers[0xF] = 0;
                    break;
                case 0x4: { // Set Vx = Vx + Vy, set VF = carry
                    uint16_t sum = static_cast<uint16_t>(registers[x] 
                        + registers[y]);
                    registers[x] = sum & 0xFF; 
                    registers[0xF] = (sum > 255) ? 1 : 0;
                    break; }
                case 0x5:  {// Set Vx = Vx - Vy, set VF = NOT borrow
                    uint8_t flag = (registers[x] >= registers[y]) ? 1 : 0;
                    registers[x] = registers[x] - registers[y];
                    registers[0xF] = flag;
                    break; }
                case 0x6: // Set Vx = Vy SHR 1
                    uint8_t flag;
                    if (registers[y] % 2 == 0) {
                        flag = 0;
                    } else {
                        flag = 1;
                    }
                    registers[x] = registers[y] >> 1;
                    registers[0xF] = flag;
                    break;
                case 0x7: { // Set Vx = Vy - Vx, set VF = Not Borrow
                    uint8_t flag = (registers[y] >= registers[x]) ? 1 : 0;
                    registers[x] = registers[y] - registers[x];
                    registers[0xF] = flag;
                    break; }
                case 0xE: { // Set Vx = Vy SHL 1
                    uint8_t flag = (registers[y] & 0x80) ? 1 : 0;
                    registers[x] = registers[y] * 2;
                    registers[0xF] = flag;
                    break; }
            }
            break;
        
        case 0x9: // Skip next instruction if Vx != Vy
            if (registers[x] != registers[y]) {
                pc += 2;
            }
            break;
        
        case 0xA: // Set I = nnn
            index_register = static_cast<uint16_t>(nnn); 
            break;
        
        case 0xB: // Jump to location nnn + V0
            pc = nnn + registers[0x0];
            break;
        
        case 0xC: { // Set Vx = random byte AND kk
            // 'static' ensures these are only initialized ONCE
            static std::default_random_engine generator(std::random_device{}()); 
            static std::uniform_int_distribution<uint8_t> distribution(0, 255);

            uint8_t random_byte = distribution(generator); // Get a number 0-255
            registers[x] = random_byte & kk; // Bitwise AND with kk
            break; }
        
        case 0xD: { // Display n-byte sprite starting at memory location 
                    // I  at (Vx, Vy), set VF = collision.
                
                if (!vblank_ready) {
                    pc -= 2;  // repeat Dxyn until next cycle
                    break;
                }
                vblank_ready = false; // this frames draw perm. is consumed

                int start_x = registers[x] % 64;
                int start_y = registers[y] % 32;

                registers[0xF] = 0; // Reset collision flag
                uint32_t pixel_color = 0xFFFFFFFF;

                for (int row = 0; row < n; row++) {
                    int target_y = start_y + row;
                    if (target_y >= 32) break;  // clip remaining rows

                    uint8_t sprite_byte = memory[index_register + row];
                    
                    for (int col = 0; col < 8; col++) {
                        if ((sprite_byte & (0x80 >> col)) != 0) {
                            int target_x = start_x + col;
                            if (target_x >= 64) break;  // clip rest of row
                    
                            int pixel_index = (target_y * 64) + target_x;

                            // Check for collision and adjust pixel 
                            if (display[pixel_index] == pixel_color) {
                                registers[0xF] = 1;
                                display[pixel_index] = 0; 
                            } else {
                                display[pixel_index] = pixel_color;
                            }
                        }
                    }
                }
            break; }

        case 0xE: 
            if (kk == 0x9E) {
                // Skip next instruction if key with val Vx is pressed
                if (keypad[registers[x]]) pc += 2;
            } 
            else if (kk == 0xA1) {
                // Skip next instruction if key with val Vx is not pressed
                if (!keypad[registers[x]]) pc += 2;
            }
            break;
        
        case 0xF:

            switch(kk) {
                // Set Vx = delay timer value
                case 0x07:
                    registers[x] = delay_timer;
                    break;

                // Wait for a key press, store the value of the key in Vx
                case 0x0A: {
                    bool key_pressed = false; 

                    for (uint8_t i = 0; i < 16; ++i) {
                        if (keypad[i]) {
                            registers[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }

                    if (!key_pressed) {
                        pc -= 2;
                    }
                    break; }
                
                // Set delay timer = Vx
                case 0x15:
                    delay_timer = registers[x];
                    break;
                
                // Set sound timer = Vx
                case 0x18:
                    sound_timer = registers[x];
                    break;
                
                // Set I = I + Vx
                case 0x1E:
                    index_register += registers[x];
                    break;
                
                // Set I = location of sprite for digit Vx
                case 0x29:
                    index_register = FONTSET_START_ADDRESS + 
                        (registers[x] * 5);
                    break;
                
                // Store BCD representation of Vx in mem locations I, I+1, and I+2
                case 0x33: {
                    int value = static_cast<int>(registers[x]);
                    memory[index_register] = (value / 100);
                    memory[index_register + 1] = (value / 10) % 10;
                    memory[index_register + 2] = value % 10;
                    break; }
                
                // Store registers V0 through Vx in memory starting at location I
                case 0x55:
                    for (int i = 0; i <= x; ++i) {
                        if (index_register + i < memory.size()) {
                            memory[index_register + i] = registers[i];
                        }
                    }
                    index_register += (x + 1);
                    break;
                
                // Read registers V0 through Vx from memory starting at location I
                case 0x65:
                    for (int i = 0; i <= x; ++i) {
                        registers[i] = memory[index_register + i];
                    }
                    index_register += (x + 1);
                    break;
            }
            break;
        default:
            break;
    }
}

void CPU::setKeyState(uint8_t key, bool isPressed) {
    if (key < 16) {
        keypad[key] = isPressed ? 1 : 0;
    }
}