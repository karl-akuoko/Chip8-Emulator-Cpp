#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>
#include <array>

class CPU
{
public:
    // Constructor: sets up initial state
    CPU();
    // Loads game into memory
    void loadROM(const std::string& filename);
    // Fetch-Decode_Execute
    void cycle();

private:
    // Hardware Components

    // 4KB of Memory
    std::array<uint8_t, 4096> memory;

    // 16 General Purpose 8-bit Registers (V0 to VF)
    std::array<uint8_t, 16> registers;

    // Index Register (I) and Program Counter (pc)
    uint16_t index_register;
    uint16_t pc;

    // Stack (16 levels) and Stack Pointer
    std::array<uint16_t, 16> stack;
    uint8_t sp;

    // Timers (counting down at 60Hz)
    uint8_t delay_timer;
    uint8_t sound_timer;

    // Display (64x32 pixels, monochrome)
    std::array<uint32_t, 64 * 32> display;

    // Internal helper function
    void decode_and_execute(uint16_t instruction);
};

#endif