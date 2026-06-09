#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_NULL : public Mapper
    {
        // There is not a cartridge inserted in the console.
    public:
        void FetchPRG(ushort Address, bool Observe) override
        {
            dataPinsAreNotFloating = false;
            // the data pins are always floating. There's no cartridge inserted!
            return;
        }

        byte FetchCHR(ushort Address, bool Observe) override
        {
            // there's no cartridge. TODO: Look into this. Supposedly this would likely be the lower 8 bits of the address bus, but CIRAM enable is also floating.
            return 0;
        }
        ushort MirrorNametable(ushort Address) override
        {
            return Address;
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            //exitIndex = p;
        }
    };
}