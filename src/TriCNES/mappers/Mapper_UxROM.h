#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_UxROM : public Mapper
    {
        // ines Mapper 2
    public:
        byte Mapper_2_BankSelect = 0;
        void FetchPRG(ushort Address, bool Observe) override
        {
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; }
            else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0x8000)
            {
                notFloating = true;
                if (Address >= 0xC000)
                {
                    ushort tempo = (ushort)(Address & 0x3FFF);
                    data = Cart->PRGROM[Cart->PRGROMLength - 0x4000 + tempo];
                }
                else
                {
                    ushort tempo = (ushort)(Address & 0x3FFF);
                    data = Cart->PRGROM[0x4000 * (Mapper_2_BankSelect & 0x0F) + tempo];
                }
            }

            if (notFloating)
            {
                EndFetchPRG(Observe, data);
            }
            return;
        }
        void StorePRG(ushort Address, byte Input) override
        {
            if (Address >= 0x8000)
            {
                Mapper_2_BankSelect = (byte)(Input & 0xF);
            }
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }
            State.push_back(Mapper_2_BankSelect);
            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }
            Mapper_2_BankSelect = State[p++];
            // exitIndex = p;
        }
    };
}