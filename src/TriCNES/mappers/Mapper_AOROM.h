#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_AOROM : public Mapper
    {
        // ines Mapper 7
    public:
        byte Mapper_7_BankSelect = 0;
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (!Cart->Emu->SeventyTwoPinConnector[49]) // CPU /A15 + /M2
            {
                CPU_DataOut = Cart->PRGROM[(0x8000 * (Mapper_7_BankSelect & 0x07) + (CPU_AddressIn & 0x7FFF)) & (Cart->PRGROMLength - 1)]; // Get the address from the ROM file. If the ROM only has $4000 bytes, this will make addresses > $BFFF mirrors of $8000 through $BFFF.
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address >= 0x8000)
            {
                Mapper_7_BankSelect = Input;
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0x8000)
            {
                return Cart->PRGROM[(0x8000 * (Mapper_7_BankSelect & 0x07) + (Address & 0x7FFF)) & (Cart->PRGROMLength - 1)]; // Get the address from the ROM file. If the ROM only has $4000 bytes, this will make addresses > $BFFF mirrors of $8000 through $BFFF.
            }
            return Cart->Emu->dataBus;
        }
        void Connector_CheckCIRAM() override
        {
            if (TiltingCart)
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = Cart->Emu->SeventyTwoPinConnector[57]; }
                if (!Cart->Emu->ConnectorPinFloating[21])
                {
                    Cart->Emu->SeventyTwoPinConnector[21] = (Mapper_7_BankSelect & 0x10) != 0;
                }
            }
            else
            {
                Cart->Emu->SeventyTwoPinConnector[56] = (Cart->Emu->PPU_AddressBus & 0x2000) == 0;
                Cart->Emu->SeventyTwoPinConnector[21] = (Mapper_7_BankSelect & 0x10) != 0;
            }
        }
        byte SnoopPPU(ushort Address) override // For debug purposes. It's a bit clunky having to set this up for every mapper with a non-NROM CIRAM setup.
        {
            if (Address < 0x2000)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                return Cart->CHRROM[CHR_Address];
            }
            else
            {
                ushort Addr = (ushort)(Address & 0x3FF);
                Addr |= (ushort)(((Mapper_7_BankSelect & 0x10) == 0) ? 0 : 0x400);
                return Cart->Emu->VRAM[Addr];
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