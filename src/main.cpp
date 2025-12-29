#include "../include/cpu.hpp"
#include <SDL2/SDL.h>
#include <iostream>


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./Chip8Emulator <ROM_PATH>" << std::endl;
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return 1;
    }

    const int SCALE = 15;
    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        64 * SCALE, 32 * SCALE, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Audio setup
    SDL_AudioSpec want;
    want.freq = 44100;          // Standard CD-quality frequency
    want.format = AUDIO_S16SYS; // 16-bit signed integers
    want.channels = 1;          // Mono sound
    want.samples = 2048;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);

    std::vector<int16_t> audioBuffer;
    int sampleRate = 44100;
    int frequency = 440; // The beep pitch
    int amplitude = 3000; // The volume

    for (int i = 0; i < sampleRate; ++i) {
        if ((i / (sampleRate / frequency / 2)) % 2 == 0) {
            audioBuffer.push_back(amplitude);
        } else {
            audioBuffer.push_back(-amplitude);
        }
    }

    CPU chip8;
    chip8.loadROM(argv[1]);

    bool running = true;
    SDL_Event event;
    uint32_t last_timer_time = SDL_GetTicks();

    while (running) {
        // Handle Events (Input)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool pressed = (event.type == SDL_KEYDOWN);
                switch (event.key.keysym.sym) {
                    case SDLK_0: chip8.setKeyState(0, pressed); break;
                    case SDLK_1: chip8.setKeyState(1, pressed); break;
                    case SDLK_2: chip8.setKeyState(2, pressed); break;
                    case SDLK_3: chip8.setKeyState(3, pressed); break;
                    case SDLK_4: chip8.setKeyState(4, pressed); break;
                    case SDLK_5: chip8.setKeyState(5, pressed); break;
                    case SDLK_6: chip8.setKeyState(6, pressed); break;
                    case SDLK_7: chip8.setKeyState(7, pressed); break;
                    case SDLK_8: chip8.setKeyState(8, pressed); break;
                    case SDLK_9: chip8.setKeyState(9, pressed); break;
                    case SDLK_a: chip8.setKeyState(0xA, pressed); break;
                    case SDLK_b: chip8.setKeyState(0xB, pressed); break;
                    case SDLK_c: chip8.setKeyState(0xC, pressed); break;
                    case SDLK_d: chip8.setKeyState(0xD, pressed); break;
                    case SDLK_e: chip8.setKeyState(0xE, pressed); break;
                    case SDLK_f: chip8.setKeyState(0xF, pressed); break;
                }
            }
        }

        // CPU Cycles (Run several cycles per frame)
        for (int i = 0; i < 15; ++i) {
            chip8.cycle();
        }

        // hardware speaker reacting to internal CPU logic 
        if (chip8.isBuzzerPlaying()) {
            SDL_PauseAudioDevice(dev, 0); // Hardware: Start the speaker
            
            // Maintain the audio buffer queue
            if (SDL_GetQueuedAudioSize(dev) < 8192) {
                SDL_QueueAudio(dev, audioBuffer.data(), audioBuffer.size() * sizeof(int16_t));
            }
        } else {
            SDL_PauseAudioDevice(dev, 1); // Hardware: Stop the speaker
            SDL_ClearQueuedAudio(dev);
        }

        // Update Timers at 60Hz
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_timer_time >= 16) { // ~60 Hz
            chip8.updateTimers();
            chip8.setVBlankReady(true);
            last_timer_time = current_time;
        }

        // Draw to Screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set color to Black
        SDL_RenderClear(renderer);                      // Clear the backbuffer

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Set color to White for pixels

        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 64; ++x) {
                // Check if the pixel at (x, y) is on
                if (chip8.getDisplay()[y * 64 + x] != 0) {
                    SDL_Rect pixel;
                    pixel.x = x * SCALE;
                    pixel.y = y * SCALE;
                    pixel.w = SCALE;
                    pixel.h = SCALE;

                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }

        SDL_RenderPresent(renderer); // Show the newly drawn frame
        SDL_Delay(1); // Sleep slightly to prevent 100% CPU usage
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}