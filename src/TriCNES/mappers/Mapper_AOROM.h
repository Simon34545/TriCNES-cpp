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
            if (!Observe)
            {
                Address = Connector_ReadCPUAddressPins();
            }
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; }
            else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0x8000)
            {
                ushort tempo = (ushort)(Address & 0x7FFF);
                data = Cart->PRGROM[(0x8000 * (Mapper_7_BankSelect & 0x07) + tempo) & (Cart->PRGROMLength - 1)]; // Get the address from the ROM file. If the ROM only has $4000 bytes, this will make addresses > $BFFF mirrors of $8000 through $BFFF.
                notFloating = true;
            }
            //open bus

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
        void Connector_CheckCIRAM() override
        {
            if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = !Cart->Emu->SeventyTwoPinConnector[57]; }
            if ((Mapper_7_BankSelect & 0x10) == 0) // show nametable 0
            {
                if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = false; }
            }
            else // show nametable 1
            {
                if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = true; }
            }
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { State.push_back(Cart->CHRROM[i]); }
            }
            State.push_back(Mapper_7_BankSelect);
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
            Mapper_7_BankSelect = State[p++];
            // exitIndex = p;
        }
    };
}