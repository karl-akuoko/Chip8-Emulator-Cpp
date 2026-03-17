# CHIP-8 Emulator

A C++17 CHIP-8 emulator built from scratch using SDL2. This project implements the core CHIP-8 virtual machine, allowing you to play classic ROMs like Pong, Tetris, and Invaders on macOS.

<img width="957" height="494" alt="image" src="https://github.com/user-attachments/assets/0c92eebc-c70c-4c5f-8850-1c91284db194" />
**Running "TicTac."**

## Features

* **Core Emulation:** Full implementation of the CHIP-8 instruction set (Opcode interpretation).
* **Graphics:** 64x32 monochrome display rendered via SDL2 (scaled 15x).
* **Sound:** Synthesized square wave beeps for game audio.
* **Timers:** Accurate 60Hz delay and sound timers.
* **MacOS Compatible:** Fully supports both Intel and Apple Silicon Macs.

## Technical Implementation

* **Architecture:** The core CPU loop executes **15 instructions per frame**.
* **Decoupled Rendering Loop:** This implementation separates the display logic from timer logic. While timers update at the standard **60Hz**, the display refreshes at **70** (~14ms intervals). This makes certain games like PONG faster and smoother. 
* **Opcode Support:** Implements the full standard CHIP-8 instruction set (35 opcodes). 
* **Graphics Pipeline:** The 64x32 display buffer is rendered using hardware-accelerated SDL2 primitives, scaled 15x for modern high-resolution monitors.
* **Audio Synthesis:** Sound does not rely on external assets; instead, the emulator generates a raw 440Hz square wave in real-time using `SDL_AudioDevice` and ring buffer management.
* **Input Handling:** Keyboard events are captured via SDL's event polling and mapped directly to the emulator's hex keypad state array, supporting multi-key presses. 

## GUI & Interface
The emulator features a graphical overlay built with `Dear ImGui`, providing a more modern user experience than traditional CLI-only emulators.

* **Dynamic ROM Loading:** A "File" menu that uses `std::filesystem` to scan the `ROMs/` directory in real-time.

* **Input Filtering:** Uses `ImGuiIO` to detect when the user is interacting with menus, preventing "ghost" inputs from affecting the game state.

* **Instant Reset:** Selecting a new ROM automatically clears the `CPU` registers and memory, allowing you to jump between games without restarting the application.


## Prerequisites (macOS)

This project is currently designed for **macOS**. You will need the following tools installed:

* **C++ Compiler:** Clang (installed via Xcode Command Line Tools).
* **CMake:** Version 3.15 or higher.
* **SDL2:** The Simple DirectMedia Layer library.

### Installing Dependencies via Homebrew
If you don't have Homebrew installed, get it from [brew.sh](https://brew.sh). Then run:

```bash
brew install cmake sdl2
```

## Build Instructions

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/karl-akuoko/Chip8-Emulator-Cpp.git
    cd Chip8-Emulator-Cpp
    ```

2.  **Navigate to the build directory:**
    ```bash
    cd build
    ```

3.  **Generate the Makefile:**
    ```bash
    cmake ..
    ```

4.  **Compile the project:**
    ```bash
    make
    ```

## Usage

1. **Launch the emulator:**
    ```bash
    ./Chip8Emulator
    ```
2. **Load a ROM:**
Use the File menu at the top of the window to select a game from the `ROMs/` folder. The emulator will automatically reset and begin execution once a file is selected.

## Included ROMs

For convenience, a selection of public domain ROMs are included in the `ROMs/` folder.

* **Source:** [Zophar's Domain](https://www.zophar.net/pdroms/chip8.html)

## Quirks & Compatibility

This emulator was primarily built using [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.5). However, as that specification contains some ambiguities and errors regarding proper behavior, the logic was rigorously cross-referenced and corrected using the [Timendus Chip-8 Test Suite](https://github.com/Timendus/chip8-test-suite).

To ensure the widest compatibility with the included ROMs, the following specific SUPER-CHIP quirks were implemented:

* **Shift Instructions (`8xy6`, `8xyE`):** Configured to modern SCHIP behavior (modifies `VX` in place).
* **Memory Load/Store (`Fx55`, `Fx65`):** Configured to modern behavior (does not increment the Index register).
* **VF Reset:** **Disabled.** (Logic instructions like AND/OR/XOR do not reset the VF flag to 0, matching SUPER-CHIP behavior).
* **Display Wait:** **Enabled.**
    * *Note:* While some modern specifications disable this, it was intentionally **enabled** in this implementation to ensure smooth operation with the included ROM files. The [Timendus Chip-8 Test Suite](https://github.com/Timendus/chip8-test-suite) shows this as disabled. This is because the refresh rate of the display was increased relative to the timers, which govern the test. 

## Controls

The original CHIP-8 used a hexadecimal keypad (0-F). This emulator maps these keys directly to your keyboard's number row and letter keys.

| CHIP-8 Key | Keyboard Key |
| :--- | :--- |
| **0 - 9** | `0` - `9` |
| **A** | `A` |
| **B** | `B` |
| **C** | `C` |
| **D** | `D` |
| **E** | `E` |
| **F** | `F` |

### Game-Specific Controls

Since CHIP-8 games were written by different authors, control schemes vary. Here are the controls for some of the popular ROMs included in this project:

| Game | Controls | Action |
| :--- | :--- | :--- |
| **PONG** | `1` / `4` | Player 1 Up / Down |
| | `C` / `D` | Player 2 Up / Down |
| **INVADERS** | `4` / `6` | Move Left / Right |
| | `5` | Shoot |
| **TETRIS** | `4` / `6` | Move Piece Left / Right |
| | `5` | Rotate Piece |
| | `8` | Drop Piece |
| **BRIX** | `4` / `6` | Move Paddle Left / Right |
| **BLITZ** | `5` | Drop Bomb |
| **TANK** | `8` / `2` | Move Down / Up |
| | `4` / `6` | Move Left / Right |
| | `5` | Fire |

## Project Structure

* `src/`: Contains the source code (`main.cpp`, `cpu.cpp`).
* `lib/`: Third party libraries, including only `Dear ImGui` as of now.
* `include/`: Contains header files (`cpu.hpp`).
* `build/`: Stores the generated executable and temporary files.
* `ROMs/`: Playable game files. 
* `CMakeLists.txt`: Build configuration.

## Future Improvements

* **Dynamic Configuration:** Implement command-line arguments (e.g., `--scale 20` or `--quirks modern`) to allow users to adjust the window size and toggle specific CHIP-8 quirks without recompiling.

## References & Resources

* **Cowgod's Chip-8 Technical Reference:** The primary specification used for this implementation.
  * [http://devernay.free.fr/hacks/chip8/C8TECH10.HTM](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.5)
* **Timendus Chip-8 Test Suite:** Essential test ROMs used for debugging and verifying opcode behavior.
  * [https://github.com/Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite)
* **Zophar's Domain:** Source for the public domain ROMs included in this project.
  * [https://www.zophar.net/pdroms/chip8.html](https://www.zophar.net/pdroms/chip8.html)

## License

MIT License

Copyright (c) 2026 Karl Akuoko
