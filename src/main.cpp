#include "FluidSimulation.h"

#include <algorithm>
#include <chrono>
#include <conio.h>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
constexpr int kWidth = 64;
constexpr int kHeight = 28;
constexpr float kTimeStep = 0.35F;
constexpr char kRamp[] = " .:-=+*#%@";

bool enableVirtualTerminalOutput() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode)) return false;
    return SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

void moveCursorToGridStart() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(output, {0, 0});
}

void draw(const FluidSimulation& fluid, int stirrerX, int stirrerY, bool terminalSupportsAnsi) {
    std::string frame;
    frame.reserve(static_cast<std::size_t>(fluid.width() * fluid.height() * 18));
    for (int y = 0; y < fluid.height(); ++y) {
        for (int x = 0; x < fluid.width(); ++x) {
            if (x == stirrerX && y == stirrerY) {
                frame += terminalSupportsAnsi ? "\x1B[97mO" : "O";
                continue;
            }
            const DyeColour dye = fluid.dyeAt(x, y);
            const float red = 1.0F - std::exp(-dye.red * 0.45F);
            const float green = 1.0F - std::exp(-dye.green * 0.45F);
            const float blue = 1.0F - std::exp(-dye.blue * 0.45F);
            const float brightness = std::max({red, green, blue});
            const int shade = std::clamp(static_cast<int>(brightness * 9.0F), 0, 9);
            if (shade == 0) {
                frame += ' ';
            } else {
                const int r = static_cast<int>(red * 255.0F);
                const int g = static_cast<int>(green * 255.0F);
                const int b = static_cast<int>(blue * 255.0F);
                if (terminalSupportsAnsi) {
                    frame += "\x1B[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
                }
                frame += kRamp[shade];
            }
        }
        frame += terminalSupportsAnsi ? "\x1B[0m\n" : "\n";
    }
    std::cout << frame << std::flush;
}

void stir(FluidSimulation& fluid, int x, int y, float pushX, float pushY,
          DyeColour colour, bool addDye) {
    // A compact circular source. Moving it gives local fluid the same impulse
    // as the user's movement direction; Space injects visible dye in place.
    for (int offsetY = -2; offsetY <= 2; ++offsetY) {
        for (int offsetX = -2; offsetX <= 2; ++offsetX) {
            if (offsetX * offsetX + offsetY * offsetY > 4) continue;
            if (addDye) fluid.addDye(x + offsetX, y + offsetY, colour, 2.2F);
            fluid.addVelocity(x + offsetX, y + offsetY, pushX, pushY);
        }
    }
}
}

int main() {
    FluidSimulation fluid(kWidth, kHeight, 0.0008F, 0.0002F);
    const bool terminalSupportsAnsi = enableVirtualTerminalOutput();
    if (!terminalSupportsAnsi) {
        std::cerr << "This program needs a terminal with ANSI support. Run it in Windows Terminal or PowerShell 7.\n";
        return 1;
    }
    std::cout << "\x1B[2J\x1B[?25l"; // Clear and hide cursor once.
    // Reserve the grid area, then write this static help only once below it.
    std::cout << std::string(kHeight, '\n');
    std::cout << "W/A/S/D: move and stir   Space: add dye   1-5: choose colour   R: reset   Q: quit\n";
    std::cout << "O is your stirrer. hold movement keys to push the fluid.\n";

    int stirrerX = kWidth / 2;
    int stirrerY = kHeight / 2;
    DyeColour selectedColour{0.15F, 0.65F, 1.0F};
    bool running = true;
    while (running) {
        // _kbhit and _getch are Windows console functions: they read keys
        // immediately, without the user needing to press Enter.
        while (_kbhit()) {
            const int key = _getch();
            int moveX = 0;
            int moveY = 0;
            bool addDye = false;

            switch (key) {
            case 'w': case 'W': moveY = -1; break;
            case 'a': case 'A': moveX = -1; break;
            case 's': case 'S': moveY = 1; break;
            case 'd': case 'D': moveX = 1; break;
            case ' ': addDye = true; break;
            case '1': selectedColour = {0.15F, 0.65F, 1.0F}; break;
            case '2': selectedColour = {1.0F, 0.16F, 0.12F}; break;
            case '3': selectedColour = {0.20F, 1.0F, 0.35F}; break;
            case '4': selectedColour = {1.0F, 0.68F, 0.10F}; break;
            case '5': selectedColour = {0.75F, 0.22F, 1.0F}; break;
            case 'r': case 'R': fluid.clear(); break;
            case 'q': case 'Q': running = false; break;
            default: break;
            }

            if (moveX != 0 || moveY != 0) {
                stirrerX = std::clamp(stirrerX + moveX, 3, kWidth - 4);
                stirrerY = std::clamp(stirrerY + moveY, 3, kHeight - 4);
                stir(fluid, stirrerX, stirrerY, moveX * 3.5F, moveY * 3.5F, selectedColour, true);
            } else if (addDye) {
                stir(fluid, stirrerX, stirrerY, 0.0F, 0.0F, selectedColour, true);
            }
        }

        fluid.step(kTimeStep);
        moveCursorToGridStart();
        draw(fluid, stirrerX, stirrerY, terminalSupportsAnsi);
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    std::cout << "\x1B[0m\x1B[?25h\n"; // Reset colour and restore cursor.
}
