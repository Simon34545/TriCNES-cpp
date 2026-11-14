#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_GxROM : public Mapper
    {
        // ines Mapper 66
    public:
        byte Mapper_66_BankSelect_CHR = 0;
        byte Mapper_66_BankSelect_PRG = 0;
        int FetchPatternAddress(ushort Address) override
        {
            return (Mapper_66_BankSelect_CHR * 0x2000 + Address) & (Cart->CHRROMLength - 1);
        }
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (CPU_AddressIn >= 0x8000)
            {
                CPU_DataOut = Cart->PRGROM[0x8000 * (Mapper_66_BankSelect_PRG)+(CPU_AddressIn & 0x7FFF)];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address >= 0x8000)
            {
                Mapper_66_BankSelect_CHR = (byte)(Input & 0x3);
                Mapper_66_BankSelect_PRG = (byte)((Input & 0x30) >> 4);
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0x8000)
            {
                return Cart->PRGROM[0x8000 * (Mapper_66_BankSelect_PRG)+(Address & 0x7FFF)];
            }
            return Cart->Emu->dataBus;
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { State.push_back(Cart->CHRROM[i]); }
            }
            State.push_back(Mapper_66_BankSelect_CHR);
            State.push_back(Mapper_66_BankSelect_PRG);
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
            Mapper_66_BankSelect_CHR = State[p++];
            Mapper_66_BankSelect_PRG = State[p++];
            // exitIndex = p;
        }
    };
}