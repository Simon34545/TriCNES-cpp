#include "TriCNES-cpp-benchmark.h"
#include "Emulator.h"

#include <chrono>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "USAGE: TriCNES-cpp-benchmark.exe rom.nes" << std::endl;
        return 0;
    }

	TriCNES::Emulator emulator;
	TriCNES::Cartridge cartridge(argv[1]);
    emulator.Cart = &cartridge;

    auto t0 = std::chrono::steady_clock::now();
    double avg = 0.0f;
    int total = 0;

    while (true) {
        emulator._CoreFrameAdvance();

        auto t1 = std::chrono::steady_clock::now();

        auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        avg += (1000000.0 / deltaTime);
        total++;

        if (total >= 60) {
            avg /= 60.0;
            printf("%.2f FPS\n", avg);
            avg = 0.0f;
            total = 0;
		}

        t0 = t1;
    }

	return 0;
}
