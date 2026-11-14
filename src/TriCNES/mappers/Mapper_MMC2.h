#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_MMC2 : public Mapper
    {
        // ines Mapper 9
    public:
        byte Mapper_9_BankSelect = 0;
        byte Mapper_9_CHR0_FD = 0;
        byte Mapper_9_CHR0_FE = 0;
        byte Mapper_9_CHR1_FD = 0;
        byte Mapper_9_CHR1_FE = 0;
        bool Mapper_9_NametableMirroring = false;
        bool Mapper_9_Latch0_FE = false;
        bool Mapper_9_Latch1_FE = false;
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (CPU_AddressIn >= 0xA000)
            {
                CPU_DataOut = Cart->PRGROM[((Cart->PRG_Size - 2) << 14) | (CPU_AddressIn & 0x7FFF)];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (CPU_AddressIn >= 0x8000)
            {
                CPU_DataOut = Cart->PRGROM[(Mapper_9_BankSelect << 13) | (CPU_AddressIn & 0x1FFF)];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address < 0xA000)
            {
                // nothing
            }
            else if (Address < 0xB000) // PRG Bank select
            {
                Mapper_9_BankSelect = (byte)(Input & 0x0F);
            }
            else if (Address < 0xC000) // CHR0 Bank select
            {
                Mapper_9_CHR0_FD = (byte)(Input & 0x1F);
            }
            else if (Address < 0xD000) // CHR0 Bank select
            {
                Mapper_9_CHR0_FE = (byte)(Input & 0x1F);
            }
            else if (Address < 0xE000) // CHR1 Bank select
            {
                Mapper_9_CHR1_FD = (byte)(Input & 0x1F);
            }
            else if (Address < 0xF000) // CHR1 Bank select
            {
                Mapper_9_CHR1_FE = (byte)(Input & 0x1F);
            }
            else // Nametable mirroring
            {
                Mapper_9_NametableMirroring = (Input & 0x1) == 1;
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0xA000)
            {
                return Cart->PRGROM[((Cart->PRG_Size - 2) << 14) | (Address & 0x7FFF)];
            }
            else if (Address >= 0x8000)
            {
                return Cart->PRGROM[(Mapper_9_BankSelect << 13) | (Address & 0x1FFF)];
            }
            return Cart->Emu->dataBus;
        }
        void AccessPPU() override
        {
            Connector_ReadPPUAddressPins();
            ushort Address = PPU_AddressIn;
            if (!Cart->Emu->SeventyTwoPinConnector[64]) // (If PPU A13 is set, we don't do anything on the cartridge)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                PPU_DataIn = Connector_ReadPPUDataPins(PPU_DataIn);
                if (!Cart->Emu->SeventyTwoPinConnector[20]) {
                    PPU_DataOut = Cart->CHRROM[CHR_Address];
                    Connector_SetUpPPUDataPins(PPU_DataOut);
                    // MMC2 has registers that do things based on the address being read.
                    if (Address == 0x0FD8)
                    {
                        Mapper_9_Latch0_FE = false;
                    }
                    else if (Address == 0x0FE8)
                    {
                        Mapper_9_Latch0_FE = true;
                    }
                    else if (Address >= 0x1FD8 && Address <= 0x1FDF)
                    {
                        Mapper_9_Latch1_FE = false;
                    }
                    else if (Address >= 0x1FE8 && Address <= 0x1FEF)
                    {
                        Mapper_9_Latch1_FE = true;
                    }
                } // Reads
                if (!Cart->Emu->SeventyTwoPinConnector[55] && Cart->UsingCHRRAM) { Cart->CHRROM[CHR_Address] = PPU_DataIn; } // Writes
            }
        }
        int FetchPatternAddress(ushort Address) override
        {
            ushort Addr = Address;
            if (Address < 0x1000) { return (Mapper_9_Latch0_FE ? Mapper_9_CHR0_FE : Mapper_9_CHR0_FD) * 0x1000 + Addr; }
            else { Addr &= 0xFFF; return (Mapper_9_Latch1_FE ? Mapper_9_CHR1_FE : Mapper_9_CHR1_FD) * 0x1000 + Addr; }
        }
        void Connector_CheckCIRAM() override
        {
            if (TiltingCart)
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = Cart->Emu->SeventyTwoPinConnector[57]; }
                if (Mapper_9_NametableMirroring) //horizontal
                {
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[61]; }
                }
                else //vertical
                {
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[62]; }
                }
            }
            else
            {
                Cart->Emu->SeventyTwoPinConnector[56] = (Cart->Emu->PPU_AddressBus & 0x2000) == 0;
                Cart->Emu->SeventyTwoPinConnector[21] = Mapper_9_NametableMirroring ? ((Cart->Emu->PPU_AddressBus & 0x800) != 0) : ((Cart->Emu->PPU_AddressBus & 0x400) != 0);
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
                Addr |= (ushort)((Mapper_9_NametableMirroring ? ((Address & 0x800) != 0) : ((Address & 0x400) != 0)) ? 0x400 : 0);
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
            State.push_back(Mapper_9_BankSelect);
            State.push_back(Mapper_9_CHR0_FD);
            State.push_back(Mapper_9_CHR0_FE);
            State.push_back(Mapper_9_CHR1_FD);
            State.push_back(Mapper_9_CHR1_FE);
            State.push_back((byte)(Mapper_9_NametableMirroring ? 1 : 0));
            State.push_back((byte)(Mapper_9_Latch0_FE ? 1 : 0));
            State.push_back((byte)(Mapper_9_Latch1_FE ? 1 : 0));
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
            Mapper_9_BankSelect = State[p++];
            Mapper_9_CHR0_FD = State[p++];
            Mapper_9_CHR0_FE = State[p++];
            Mapper_9_CHR1_FD = State[p++];
            Mapper_9_CHR1_FE = State[p++];
            Mapper_9_NametableMirroring = (State[p++] & 1) == 1;
            Mapper_9_Latch0_FE = (State[p++] & 1) == 1;
            Mapper_9_Latch1_FE = (State[p++] & 1) == 1;
            // exitIndex = p;
        }
    };
}