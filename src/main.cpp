#include "../include/cpu.hpp"
#include "../lib/imgui/imgui.h"
#include "../lib/imgui/backends/imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <filesystem> // For scanning the ROMs folder
#include <string>
#include <SDL2/SDL.h>
#include <iostream>

namespace fs = std::filesystem; 

int main(int argc, char** argv) {
    if (argc != 1) {
        std::cout << "Usage: ./Chip8Emulator" << std::endl;
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return 1;
    }

    const int SCALE = 15;
    // 20 extra scaled pixels on the height to make room for the menu bar
    SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        64 * SCALE, 32 * SCALE + 20, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);
    
    // Imgui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    
    // Audio setup
    SDL_AudioSpec want;
    SDL_zero(want);
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
    bool romLoaded = false;

    bool running = true;
    SDL_Event event;
    uint32_t last_timer_time = SDL_GetTicks();
    uint32_t last_vblank_time = SDL_GetTicks();

    while (running) {
        // Handle Events (Input)
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) running = false;

            // Only capture game keys if no typing in the game window
            if (!io.WantCaptureKeyboard) {
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
        }

        // Start Imgui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Create the Menu Bar at the top
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                // Look into the /ROMs folder
                if (fs::exists("../ROMs/")) {
                    for (const auto& entry : fs::directory_iterator("../ROMs/")) {
                        // Create a clickable menu item for each file found
                        if (ImGui::MenuItem(entry.path().filename().string().c_str())) {
                            chip8.reset(); // Clear old game data
                            chip8.loadROM(entry.path().string().c_str());
                            romLoaded = true;
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) running = false;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (romLoaded) {
            // CPU Cycles (Run several cycles per frame)
            for (int i = 0; i < 15; ++i) {
                chip8.cycle();
            }
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

        if (romLoaded) {
            // Update Timers at 60Hz
            uint32_t current_time = SDL_GetTicks();
            if (current_time - last_timer_time >= 16) { // ~60 hz
                chip8.updateTimers();
                chip8.setVBlankReady(true);
                last_timer_time = current_time;
            }

            // Display Hz controller
            if (current_time - last_vblank_time >= 14) { // ~70 Hz
                chip8.setVBlankReady(true);
                last_vblank_time = current_time;
            }
        }

        // Draw to Screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set color to Black
        SDL_RenderClear(renderer);                      // Clear the backbuffer

        if (romLoaded) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Set color to White for pixels
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 64; ++x) {
                    // Check if the pixel at (x, y) is on
                    if (chip8.getDisplay()[y * 64 + x] != 0) {
                        SDL_Rect pixel;
                        pixel.x = x * SCALE;
                        // Added 20 to 'y' so game draws below the menu bar
                        pixel.y = (y * SCALE) + 20;
                        pixel.w = SCALE;
                        pixel.h = SCALE;

                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
        }

        // Render Imgui
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer); // Show the newly drawn frame
        SDL_Delay(1); // Sleep slightly to prevent 100% CPU usage
    }
    
    // Imgui Cleanup 
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // SDL cleanuo
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}