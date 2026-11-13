#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_NULL : public Mapper
    {
        // There is not a cartridge inserted in the console.
    public:
        void FetchCPU() override
        {
            // the data pins are always floating. There's no cartridge inserted!
            return;
        }

        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            return Cart->Emu->dataBus; // There is nothing inserted here. The CPU *must* see open bus.
        }

        void AccessPPU() override
        {
            // the data pins are always floating. There's no cartridge inserted!
            return;
        }

        byte SnoopPPU(ushort Address) override // For debug purposes. It's a bit clunky having to set this up for every mapper with a non-NROM CIRAM setup.
        {
            return (byte)Cart->Emu->addressBus; // There is nothing inserted here. The PPU *must* see open bus.
        }

        int FetchPatternAddress(ushort Address) override
        {
            // there's no cartridge. TODO: Look into this. Supposedly this would likely be the lower 8 bits of the address bus, but CIRAM enable is also floating.
            return 0;
        }
        void Connector_CheckCIRAM() override
        {
            // CIRAM is left floating. There's no cartridge inserted!
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
        bool CheckCIC() override
        {
            return false; // There's no CIC chip!
        }
    };
}