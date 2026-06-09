#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_AOROM : public Mapper
    {
        // ines Mapper 7
    public:
        byte Mapper_7_BankSelect = 0;
        void FetchPRG(ushort Address, bool Observe) override
        {
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; }
            else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0x8000)
            {
                dataPinsAreNotFloating = true;
                ushort tempo = (ushort)(Address & 0x7FFF);
                dataBus = Cart->PRGROM[(0x8000 * (Mapper_7_BankSelect & 0x07) + tempo) & (Cart->PRGROMLength - 1)];
            }
            // AOROM doesn't have any PRG RAM

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
                Mapper_7_BankSelect = Input;
            }
        }
        ushort MirrorNametable(ushort Address) override
        {
            if ((Mapper_7_BankSelect & 0x10) == 0) // show nametable 0
            {
                Address &= 0x33FF;
            }
            else // show nametable 1
            {
                Address &= 0x33FF;
                Address |= 0x400;
            }
            return Address;
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }
            State.push_back(Mapper_7_BankSelect);
            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }
            Mapper_7_BankSelect = State[p++];
            // exitIndex = p;
        }
    };
}