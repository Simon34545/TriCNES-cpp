#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_MMC3 : public Mapper
    {
        // ines Mapper 4
    public:
        byte Mapper_4_8000 = 0;      // The value written to $8000 (or any even address between $8000 and $9FFE)
        byte Mapper_4_BankA = 0;     // The PRG bank between $A000 and $BFFF
        byte Mapper_4_Bank8C = 0;    // The PRG bank that could either be at $8000 through 9FFF, or $C000 through $DFFF
        byte Mapper_4_CHR_2K0 = 0;
        byte Mapper_4_CHR_2K8 = 0;
        byte Mapper_4_CHR_1K0 = 0;
        byte Mapper_4_CHR_1K4 = 0;
        byte Mapper_4_CHR_1K8 = 0;
        byte Mapper_4_CHR_1KC = 0;
        byte Mapper_4_IRQLatch = 0;
        byte Mapper_4_IRQCounter = 0;
        bool Mapper_4_EnableIRQ = false;
        bool Mapper_4_ReloadIRQCounter = false;
        bool Mapper_4_NametableMirroring = false; // MMC3 has it's own way of controlling how the nametables are mirrored.
        byte Mapper_4_PRGRAMProtect = 0;
        byte Mapper_4_M2Filter = 0;
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[75]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (CPU_AddressIn >= 0xE000) // This bank is fixed the the final PRG bank of the ROM
            {
                CPU_DataOut = Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (CPU_AddressIn & 0x3FFF)];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (CPU_AddressIn >= 0xC000)
            {
                if ((Mapper_4_8000 & 0x40) == 0x40)
                {
                    //$C000 swappable
                    CPU_DataOut = Cart->PRGROM[(Mapper_4_Bank8C << 13) | (CPU_AddressIn & 0x1FFF)];
                }
                else
                {
                    //$8000 swappable
                    CPU_DataOut = Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (CPU_AddressIn & 0x1FFF)];
                }
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (CPU_AddressIn >= 0xA000)
            {
                //$8000 swappable
                CPU_DataOut = Cart->PRGROM[(Mapper_4_BankA << 13) | (CPU_AddressIn & 0x1FFF)];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (CPU_AddressIn >= 0x8000)
            {
                if ((Mapper_4_8000 & 0x40) == 0x40)
                {
                    //$8000 swappable
                    CPU_DataOut = Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (CPU_AddressIn & 0x1FFF)];
                }
                else
                {
                    //$C000 swappable
                    CPU_DataOut = Cart->PRGROM[(Mapper_4_Bank8C << 13) | (CPU_AddressIn & 0x1FFF)];
                }
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (CPU_AddressIn >= 0x6000)
            {
                if (Cart->SubMapper == 1) // MMC6
                {
                    if ((Mapper_4_8000 & 0x20) != 0)
                    {
                        // MMC6 differs from MMC3 since there's only 1Kib of PRG RAM
                        if (CPU_AddressIn >= 0x7000 && CPU_AddressIn <= 0x71FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x20) != 0)
                            {
                                CPU_DataOut = Cart->PRGRAM[CPU_AddressIn & 0x3FF];
                                Connector_SetUpCPUDataPins(CPU_DataOut);
                            }
                        }
                        else if (CPU_AddressIn >= 0x7200 && CPU_AddressIn <= 0x73FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x80) != 0)
                            {
                                CPU_DataOut = Cart->PRGRAM[CPU_AddressIn & 0x3FF];
                                Connector_SetUpCPUDataPins(CPU_DataOut);
                            }
                        }
                    }
                }
                else
                {
                    if ((Mapper_4_PRGRAMProtect & 0x80) != 0)
                    {
                        CPU_DataOut = Cart->PRGRAM[CPU_AddressIn & 0x1FFF];
                        Connector_SetUpCPUDataPins(CPU_DataOut);
                    }
                }
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address < 0x8000)
            {   //Battery backed RAM

                if (Cart->SubMapper == 1) // MMC6
                {
                    // MMC6 differs from MMC3 since there's only 1Kib of PRG RAM
                    if ((Mapper_4_8000 & 0x20) != 0)
                    {
                        if (Address >= 0x7000 && Address <= 0x71FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x10) == 0)
                            {
                                Cart->PRGRAM[Address & 0x3FF] = Input;

                            }
                        }
                        else if (Address >= 0x7200 && Address <= 0x73FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x40) == 0)
                            {
                                Cart->PRGRAM[Address & 0x3FF] = Input;
                            }
                        }
                    }
                }
                else if ((Mapper_4_PRGRAMProtect & 0xC0) == 0x80) // bit 7 enables PRG RAM, bit 6 disables writing there.
                {
                    Cart->PRGRAM[Address & 0x1FFF] = Input;
                }
                return;
            }
            else
            {
                ushort tempo = (ushort)(Address & 0xE001);
                byte mode;
                switch (tempo)
                {
                case 0x8000:
                    Mapper_4_8000 = Input;
                    return;
                case 0x8001:
                    mode = (byte)(Mapper_4_8000 & 7);
                    switch (mode)
                    {
                    case 0: //PPU ($0000 - $07FF) ?+ $1000
                        Mapper_4_CHR_2K0 = (byte)(Input & 0xFE);
                        return;
                    case 1: //PPU ($0800 - $0FFF) ?+ $1000
                        Mapper_4_CHR_2K8 = (byte)(Input & 0xFE);
                        return;
                    case 2: //PPU ($1000 - $13FF) ?- $1000
                        Mapper_4_CHR_1K0 = Input;
                        return;
                    case 3: //PPU ($1400 - $17FF) ?- $1000
                        Mapper_4_CHR_1K4 = Input;
                        return;
                    case 4: //PPU ($1800 - $1BFF) ?- $1000
                        Mapper_4_CHR_1K8 = Input;
                        return;
                    case 5: //PPU ($1C00 - $1FFF) ?- $1000
                        Mapper_4_CHR_1KC = Input;
                        return;
                    case 6: //PRG ($8000 - $9FFF) ?+ 0x4000
                        Mapper_4_Bank8C = (byte)(Input & (Cart->PRG_Size * 2 - 1));
                        return;
                    case 7: //PRG ($A000 - $BFFF)
                        Mapper_4_BankA = (byte)(Input & (Cart->PRG_Size * 2 - 1));
                        return;
                    }
                    return;
                case 0xA000:
                    Mapper_4_NametableMirroring = (Input & 1) == 1;
                    return;
                case 0xA001:
                    Mapper_4_PRGRAMProtect = Input;
                    return;
                case 0xC000:
                    Mapper_4_IRQLatch = Input;
                    return;
                case 0xC001:
                    Mapper_4_IRQCounter = 0xFF;
                    Mapper_4_ReloadIRQCounter = true;
                    return;
                case 0xE000:
                    Mapper_4_EnableIRQ = false;
                    Connector_IRQPin(false); // Acknowledge the IRQ
                    return;
                case 0xE001:
                    Mapper_4_EnableIRQ = true;
                    return;
                }
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0xE000) // This bank is fixed the the final PRG bank of the ROM
            {
                return Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (Address & 0x3FFF)];
            }
            else if (Address >= 0xC000)
            {
                if ((Mapper_4_8000 & 0x40) == 0x40)
                {
                    //$C000 swappable
                    return Cart->PRGROM[(Mapper_4_Bank8C << 13) | (Address & 0x1FFF)];
                }
                else
                {
                    //$8000 swappable
                    return Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (Address & 0x1FFF)];
                }
            }
            else if (Address >= 0xA000)
            {
                //$8000 swappable
                return Cart->PRGROM[(Mapper_4_BankA << 13) | (Address & 0x1FFF)];
            }
            else if (Address >= 0x8000)
            {
                if ((Mapper_4_8000 & 0x40) == 0x40)
                {
                    //$8000 swappable
                    return Cart->PRGROM[(Cart->PRG_SizeMinus1 << 14) | (Address & 0x1FFF)];
                }
                else
                {
                    //$C000 swappable
                    return Cart->PRGROM[(Mapper_4_Bank8C << 13) | (Address & 0x1FFF)];
                }
            }
            else if (Address >= 0x6000)
            {
                if (Cart->SubMapper == 1) // MMC6
                {
                    if ((Mapper_4_8000 & 0x20) != 0)
                    {
                        // MMC6 differs from MMC3 since there's only 1Kib of PRG RAM
                        if (Address >= 0x7000 && Address <= 0x71FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x20) != 0)
                            {
                                return Cart->PRGRAM[Address & 0x3FF];
                            }
                        }
                        else if (Address >= 0x7200 && Address <= 0x73FF)
                        {
                            if ((Mapper_4_PRGRAMProtect & 0x80) != 0)
                            {
                                return Cart->PRGRAM[Address & 0x3FF];
                            }
                        }
                    }
                }
                else
                {
                    if ((Mapper_4_PRGRAMProtect & 0x80) != 0)
                    {
                        return Cart->PRGRAM[Address & 0x1FFF];
                    }
                }
            }
            return Cart->Emu->dataBus;
        }
        int FetchPatternAddress(ushort Address) override
        {
            //Writes to $8000 determine the mode, writes to $8001 determine the banks
            if ((Mapper_4_8000 & 0x80) == 0) // bit 7 of the previous write to $8000 determines which pattern table is 2 2kb banks, and which is 4 1kb banks.
            {
                if (Address < 0x800)                         { return (Mapper_4_CHR_2K0 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1000) { Address &= 0x7FF; return (Mapper_4_CHR_2K8 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1400) { Address &= 0x3FF; return (Mapper_4_CHR_1K0 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1800) { Address &= 0x3FF; return (Mapper_4_CHR_1K4 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1C00) { Address &= 0x3FF; return (Mapper_4_CHR_1K8 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else                       { Address &= 0x3FF; return (Mapper_4_CHR_1KC * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            }
            else
            {
                if (Address < 0x400)                         { return (Mapper_4_CHR_1K0 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x800)  { Address &= 0x3FF; return (Mapper_4_CHR_1K4 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0xC00)  { Address &= 0x3FF; return (Mapper_4_CHR_1K8 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1000) { Address &= 0x3FF; return (Mapper_4_CHR_1KC * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else if (Address < 0x1800) { Address &= 0x7FF; return (Mapper_4_CHR_2K0 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
                else                       { Address &= 0x7FF; return (Mapper_4_CHR_2K8 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            }
        }
        void Connector_CheckCIRAM() override
        {
            if (Cart->AlternativeNametableArrangement)
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = !Cart->Emu->SeventyTwoPinConnector[57] && !Cart->Emu->SeventyTwoPinConnector[61]; }
            }
            else
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = !Cart->Emu->SeventyTwoPinConnector[57]; }
            }

            if (Mapper_4_NametableMirroring) //horizontal
            {
                if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[61]; }
            }
            else //vertical
            {
                if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[62]; }
            }
        }
        byte SnoopPPU(ushort Address) override // For debug purposes. It's a bit clunky having to set this up for every mapper with a non-NROM CIRAM setup.
        {
            if (Address < 0x2000)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                return Cart->CHRROM[CHR_Address];
            }
            else if (Cart->AlternativeNametableArrangement && Address >= 0x2800) // Unless we have an extra Nametable
            {
                Address &= 0x7FF; // I don't believe there are any carts that have an alternate nametable arrangement that aren't set up this way.
                return Cart->PRGVRAM[Address];
            }
            else
            {
                ushort Addr = (ushort)(Address & 0x3FF);
                Addr |= (ushort)((Mapper_4_NametableMirroring ? ((Address & 0x800) != 0) : ((Address & 0x400) != 0)) ? 0x400 : 0);
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
            if (Cart->PRGVRAM != nullptr)
            {
                for (int i = 0; i < Cart->PRGVRAMLength; i++) { State.push_back(Cart->PRGVRAM[i]); }
            }
            State.push_back(Mapper_4_8000);
            State.push_back(Mapper_4_BankA);
            State.push_back(Mapper_4_Bank8C);
            State.push_back(Mapper_4_CHR_2K0);
            State.push_back(Mapper_4_CHR_2K8);
            State.push_back(Mapper_4_CHR_1K0);
            State.push_back(Mapper_4_CHR_1K4);
            State.push_back(Mapper_4_CHR_1K8);
            State.push_back(Mapper_4_CHR_1KC);
            State.push_back(Mapper_4_IRQLatch);
            State.push_back(Mapper_4_IRQCounter);
            State.push_back((byte)(Mapper_4_EnableIRQ ? 1 : 0));
            State.push_back((byte)(Mapper_4_ReloadIRQCounter ? 1 : 0));
            State.push_back((byte)(Mapper_4_NametableMirroring ? 1 : 0));
            State.push_back(Mapper_4_PRGRAMProtect);
            State.push_back(Mapper_4_M2Filter);
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
            if (Cart->PRGVRAM != nullptr)
            {
                for (int i = 0; i < Cart->PRGVRAMLength; i++) { Cart->PRGVRAM[i] = State[p++]; }
            }
            Mapper_4_8000 = State[p++];
            Mapper_4_BankA = State[p++];
            Mapper_4_Bank8C = State[p++];
            Mapper_4_CHR_2K0 = State[p++];
            Mapper_4_CHR_2K8 = State[p++];
            Mapper_4_CHR_1K0 = State[p++];
            Mapper_4_CHR_1K4 = State[p++];
            Mapper_4_CHR_1K8 = State[p++];
            Mapper_4_CHR_1KC = State[p++];
            Mapper_4_IRQLatch = State[p++];
            Mapper_4_IRQCounter = State[p++];
            Mapper_4_EnableIRQ = (State[p++] & 1) == 1;
            Mapper_4_ReloadIRQCounter = (State[p++] & 1) == 1;
            Mapper_4_NametableMirroring = (State[p++] & 1) == 1;
            Mapper_4_PRGRAMProtect = State[p++];
            Mapper_4_M2Filter = State[p++];
            // exitIndex = p;
        }
        void PPUClock() override
        {
            // if bit 12 of the ppu address bus (A12) changes:
            if (!Cart->Emu->PPU_A12_Prev && ((Cart->Emu->PPU_AddressBus & 0b0001000000000000) != 0) && Mapper_4_M2Filter == 3)
            {
                if (Mapper_4_ReloadIRQCounter)
                {
                    // If we're reloading the IRQ counter
                    Mapper_4_IRQCounter = Mapper_4_IRQLatch; // The latch is the reset value.
                    Mapper_4_ReloadIRQCounter = false;
                    if (Mapper_4_IRQCounter == 0)  // if the latch is set to 0, you need to enable the IRQ.
                    {
                        if (Mapper_4_EnableIRQ) // if setting the value to zero, run an IRQ
                        {
                            Connector_IRQPin(true); // Run an IRQ!
                        }
                    }
                }
                else
                {
                    // decrement the counter
                    Mapper_4_IRQCounter--;
                    if (Mapper_4_IRQCounter == 0) // if decrementing the counter moved it to 0...
                    {
                        if (Mapper_4_EnableIRQ) // and the MMC3 IRQ is enabled...
                        {
                            Connector_IRQPin(true); // Run an IRQ!
                        }
                    }
                    else if (Mapper_4_IRQCounter == 255) // if the counter underflows...
                    {
                        Mapper_4_IRQCounter = Mapper_4_IRQLatch; // reset the irq counter
                        if (Mapper_4_IRQCounter == 0)  // if the latch is set to 0, you need to enable the IRQ... again
                        {
                            if (Mapper_4_EnableIRQ)
                            {
                                Connector_IRQPin(true); // Run an IRQ!
                            }
                        }
                    }

                }
            }
            if ((Cart->Emu->PPU_AddressBus & 0b0001000000000000) != 0)
            {
                Mapper_4_M2Filter = 0;
            }
        }
        void CPUClockRise() override
        {
            if ((Cart->Emu->PPU_AddressBus & 0b0001000000000000) == 0)
            {
                if (Mapper_4_M2Filter < 3)
                {
                    Mapper_4_M2Filter++;
                }
            }
        }
    };
}