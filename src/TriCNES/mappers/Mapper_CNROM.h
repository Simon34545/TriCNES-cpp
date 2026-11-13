#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_CNROM : public Mapper
    {
        // ines Mapper 3
    public:
        byte Mapper_3_CHRBank = 0;
        void StorePRG(ushort Address, byte Input) override
        {
            if (Address >= 0x8000)
            {
                Mapper_3_CHRBank = (byte)(Input & 0x3);
            }
        }
        int FetchPatternAddress(ushort Address) override
        {
            return (Mapper_3_CHRBank * 0x2000 + Address) & (Cart->CHRROMLength - 1);
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { State.push_back(Cart->CHRROM[i]); }
            }
            State.push_back(Mapper_3_CHRBank);
            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { Cart->CHRROM[i] = State[p++]; }
            }
            Mapper_3_CHRBank = State[p++];
            // exitIndex = p;
        }
    };
}