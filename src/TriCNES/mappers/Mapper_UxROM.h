#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_UxROM : public Mapper
    {
        // ines Mapper 2
    public:
        byte Mapper_2_BankSelect = 0;
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (CPU_AddressIn >= 0x8000)
            {
                if (CPU_AddressIn >= 0xC000)
                {
                    CPU_DataOut = Cart->PRGROM[Cart->PRGROMLength - 0x4000 + (CPU_AddressIn & 0x3FFF)];
                }
                else
                {
                    CPU_DataOut = Cart->PRGROM[0x4000 * (Mapper_2_BankSelect & 0x0F) + (CPU_AddressIn & 0x3FFF)];
                }
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address >= 0x8000)
            {
                Mapper_2_BankSelect = (byte)(Input & 0xF);
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0x8000)
            {
                if (CPU_AddressIn >= 0xC000)
                {
                    return Cart->PRGROM[Cart->PRGROMLength - 0x4000 + (Address & 0x3FFF)];
                }
                return Cart->PRGROM[0x4000 * (Mapper_2_BankSelect & 0x0F) + (Address & 0x3FFF)];
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
            State.push_back(Mapper_2_BankSelect);
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
            Mapper_2_BankSelect = State[p++];
            // exitIndex = p;
        }
    };
}