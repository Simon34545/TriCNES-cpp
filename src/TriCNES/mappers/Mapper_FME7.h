#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_FME7 : public Mapper
    {
        // ines Mapper 69
    public:
        byte Mapper_69_CMD = 0;
        byte Mapper_69_CHR_1K0 = 0;
        byte Mapper_69_CHR_1K1 = 0;
        byte Mapper_69_CHR_1K2 = 0;
        byte Mapper_69_CHR_1K3 = 0;
        byte Mapper_69_CHR_1K4 = 0;
        byte Mapper_69_CHR_1K5 = 0;
        byte Mapper_69_CHR_1K6 = 0;
        byte Mapper_69_CHR_1K7 = 0;
        byte Mapper_69_Bank_6 = 0;
        bool Mapper_69_Bank_6_isRAM = false;
        bool Mapper_69_Bank_6_isRAMEnabled = false;
        byte Mapper_69_Bank_8 = 0;
        byte Mapper_69_Bank_A = 0;
        byte Mapper_69_Bank_C = 0;
        byte Mapper_69_NametableMirroring = 0; // 0 = Vertical              1 = Horizontal            2 = One Screen Mirroring from $2000 ("1ScA")            3 = One Screen Mirroring from $2400 ("1ScB")
        bool Mapper_69_EnableIRQ = false;
        bool Mapper_69_EnableIRQCounterDecrement = false;
        ushort Mapper_69_IRQCounter = 0; // When enabled the 16-bit IRQ counter is decremented once per CPU cycle. When the IRQ counter is decremented from $0000 to $FFFF an IRQ is generated.
        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (CPU_AddressIn >= 0x6000)
            {
                ushort tempo = (ushort)(CPU_AddressIn % 0x2000);
                if (CPU_AddressIn >= 0x6000)
                {
                    //actions
                    if (CPU_AddressIn < 0x8000)
                    {
                        if (Mapper_69_Bank_6_isRAM)
                        {
                            if (Mapper_69_Bank_6_isRAMEnabled)
                            {
                                CPU_DataOut = Cart->PRGRAM[CPU_AddressIn & 0x1FFF];

                            }
                        }
                        else
                        {   //read from ROM
                            CPU_DataOut = Cart->PRGROM[(Mapper_69_Bank_6 * 0x2000 + tempo) % Cart->PRGROMLength];

                        }
                    }
                    else if (CPU_AddressIn < 0xA000)
                    {
                        CPU_DataOut = Cart->PRGROM[(Mapper_69_Bank_8 * 0x2000 + tempo) % Cart->PRGROMLength];
                        Connector_SetUpCPUDataPins(CPU_DataOut);
                    }
                    else if (CPU_AddressIn < 0xC000)
                    {
                        CPU_DataOut = Cart->PRGROM[(Mapper_69_Bank_A * 0x2000 + tempo) % Cart->PRGROMLength];
                        Connector_SetUpCPUDataPins(CPU_DataOut);
                    }
                    else if (CPU_AddressIn < 0xE000)
                    {
                        CPU_DataOut = Cart->PRGROM[(Mapper_69_Bank_C * 0x2000 + tempo) % Cart->PRGROMLength];
                        Connector_SetUpCPUDataPins(CPU_DataOut);
                    }
                    else
                    {
                        CPU_DataOut = Cart->PRGROM[Cart->PRGROMLength - 0x2000 + tempo];
                        Connector_SetUpCPUDataPins(CPU_DataOut);
                    }
                }
            }

            return;
        }
        void StoreCPU(ushort Address, byte Input) override
        {
            if (Address >= 0x6000)
            {
                //actions
                if (Address < 0x8000)
                {
                    if (Mapper_69_Bank_6_isRAM)
                    {
                        if (Mapper_69_Bank_6_isRAMEnabled)
                        {
                            //writing to RAM
                            Cart->PRGRAM[Address & 0x1FFF] = Input;
                        } //else, writing to open bus
                    } //else it's ROM. writing here does nothing.
                }
                else if (Address < 0xA000)
                {
                    Mapper_69_CMD = (byte)(Input & 0x0F);
                }
                else if (Address < 0xC000)
                {
                    switch (Mapper_69_CMD)
                    {
                    case 0: Mapper_69_CHR_1K0 = Input; break;
                    case 1: Mapper_69_CHR_1K1 = Input; break;
                    case 2: Mapper_69_CHR_1K2 = Input; break;
                    case 3: Mapper_69_CHR_1K3 = Input; break;
                    case 4: Mapper_69_CHR_1K4 = Input; break;
                    case 5: Mapper_69_CHR_1K5 = Input; break;
                    case 6: Mapper_69_CHR_1K6 = Input; break;
                    case 7: Mapper_69_CHR_1K7 = Input; break;
                    case 8: Mapper_69_Bank_6 = (byte)(Input & 0x3F); Mapper_69_Bank_6_isRAM = (Input & 0x40) != 0; Mapper_69_Bank_6_isRAMEnabled = (Input & 0x80) != 0; break;
                    case 9: Mapper_69_Bank_8 = (byte)(Input & 0x3F); break;
                    case 10: Mapper_69_Bank_A = (byte)(Input & 0x3F); break;
                    case 11: Mapper_69_Bank_C = (byte)(Input & 0x3F); break;
                    case 12: Mapper_69_NametableMirroring = (byte)(Input & 0x3); break;
                    case 13: Mapper_69_EnableIRQ = (Input & 0x1) != 0; Mapper_69_EnableIRQCounterDecrement = (Input & 0x80) != 0; Connector_IRQPin(false); break;
                    case 14: Mapper_69_IRQCounter = (ushort)((Mapper_69_IRQCounter & 0xFF00) | Input); break;
                    case 15: Mapper_69_IRQCounter = (ushort)((Mapper_69_IRQCounter & 0xFF) | (Input << 8)); break;
                    }
                } // else do nothing
            }
        }
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0x6000)
            {
                ushort tempo = (ushort)(Address % 0x2000);
                if (Address >= 0x6000)
                {
                    //actions
                    if (Address < 0x8000)
                    {
                        if (Mapper_69_Bank_6_isRAM)
                        {
                            if (Mapper_69_Bank_6_isRAMEnabled)
                            {
                                return Cart->PRGRAM[Address & 0x1FFF];
                            }
                        }
                        else
                        {   //read from ROM
                            return Cart->PRGROM[(Mapper_69_Bank_6 * 0x2000 + tempo) % Cart->PRGROMLength];
                        }
                    }
                    else if (Address < 0xA000)
                    {
                        return Cart->PRGROM[(Mapper_69_Bank_8 * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else if (Address < 0xC000)
                    {
                        return Cart->PRGROM[(Mapper_69_Bank_A * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else if (Address < 0xE000)
                    {
                        return Cart->PRGROM[(Mapper_69_Bank_C * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else
                    {
                        return Cart->PRGROM[Cart->PRGROMLength - 0x2000 + tempo];
                    }
                }
            }
            return Cart->Emu->dataBus;
        }
        int FetchPatternAddress(ushort Address) override
        {
            if (Address < 0x400) { return (Mapper_69_CHR_1K0 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0x800) { Address &= 0x3FF; return (Mapper_69_CHR_1K1 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0xC00) { Address &= 0x3FF; return (Mapper_69_CHR_1K2 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0x1000) { Address &= 0x3FF; return (Mapper_69_CHR_1K3 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0x1400) { Address &= 0x3FF; return (Mapper_69_CHR_1K4 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0x1800) { Address &= 0x3FF; return (Mapper_69_CHR_1K5 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else if (Address < 0x1C00) { Address &= 0x3FF; return (Mapper_69_CHR_1K6 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
            else { Address &= 0x3FF; return (Mapper_69_CHR_1K7 * 0x400 + Address) & (Cart->CHRROMLength - 1); }
        }
        void Connector_CheckCIRAM() override
        {
            if (TiltingCart)
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = Cart->Emu->SeventyTwoPinConnector[57]; }
                switch (Mapper_69_NametableMirroring)
                {
                case 0: //vertical
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[62]; }
                    break;
                case 1: //horizontal
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->Emu->SeventyTwoPinConnector[61]; }
                    break;
                case 2: //one-screen A
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = false; }
                    break;
                case 3: //one-screen B
                    if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = true; }
                    break;
                }
            }
            else
            {
                Cart->Emu->SeventyTwoPinConnector[56] = (Cart->Emu->PPU_AddressBus & 0x2000) == 0;
                switch (Mapper_69_NametableMirroring)
                {
                case 0: //vertical
                    Cart->Emu->SeventyTwoPinConnector[21] = (Cart->Emu->PPU_AddressBus & 0x400) != 0;
                    break;
                case 1: //horizontal
                    Cart->Emu->SeventyTwoPinConnector[21] = (Cart->Emu->PPU_AddressBus & 0x800) != 0;
                    break;
                case 2: //one-screen A
                    Cart->Emu->SeventyTwoPinConnector[21] = false;
                    break;
                case 3: //one-screen B
                    Cart->Emu->SeventyTwoPinConnector[21] = true;
                    break;
                }
            }
        }
        byte SnoopPPU(ushort Address) // For debug purposes. It's a bit clunky having to set this up for every mapper with a non-NROM CIRAM setup.
        {
            if (Address < 0x2000)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                return Cart->CHRROM[CHR_Address];
            }
            else
            {
                ushort Addr = (ushort)(Address & 0x3FF);
                switch (Mapper_69_NametableMirroring)
                {
                case 0: //vertical
                    Addr |= (ushort)(((Address & 0x400) != 0) ? 0x400 : 0);
                    break;
                case 1: //horizontal
                    Addr |= (ushort)(((Address & 0x800) != 0) ? 0x400 : 0);
                    break;
                case 2: //one-screen A
                    break;
                case 3: //one-screen B
                    Addr |= 0x400;
                    break;
                }
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
            State.push_back(Mapper_69_CMD);
            State.push_back(Mapper_69_CHR_1K0);
            State.push_back(Mapper_69_CHR_1K1);
            State.push_back(Mapper_69_CHR_1K2);
            State.push_back(Mapper_69_CHR_1K3);
            State.push_back(Mapper_69_CHR_1K4);
            State.push_back(Mapper_69_CHR_1K5);
            State.push_back(Mapper_69_CHR_1K6);
            State.push_back(Mapper_69_CHR_1K7);
            State.push_back(Mapper_69_Bank_6);
            State.push_back((byte)(Mapper_69_Bank_6_isRAM ? 1 : 0));
            State.push_back((byte)(Mapper_69_Bank_6_isRAMEnabled ? 1 : 0));
            State.push_back(Mapper_69_Bank_8);
            State.push_back(Mapper_69_Bank_A);
            State.push_back(Mapper_69_Bank_C);
            State.push_back(Mapper_69_NametableMirroring);
            State.push_back((byte)(Mapper_69_EnableIRQ ? 1 : 0));
            State.push_back((byte)(Mapper_69_EnableIRQCounterDecrement ? 1 : 0));
            State.push_back((byte)Mapper_69_IRQCounter);
            State.push_back((byte)(Mapper_69_IRQCounter >> 8));
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
            Mapper_69_CMD = State[p++];
            Mapper_69_CHR_1K0 = State[p++];
            Mapper_69_CHR_1K1 = State[p++];
            Mapper_69_CHR_1K2 = State[p++];
            Mapper_69_CHR_1K3 = State[p++];
            Mapper_69_CHR_1K4 = State[p++];
            Mapper_69_CHR_1K5 = State[p++];
            Mapper_69_CHR_1K6 = State[p++];
            Mapper_69_CHR_1K7 = State[p++];
            Mapper_69_Bank_6 = State[p++];
            Mapper_69_Bank_6_isRAM = (State[p++] & 1) == 1;
            Mapper_69_Bank_6_isRAMEnabled = (State[p++] & 1) == 1;
            Mapper_69_Bank_8 = State[p++];
            Mapper_69_Bank_A = State[p++];
            Mapper_69_Bank_C = State[p++];
            Mapper_69_NametableMirroring = State[p++];
            Mapper_69_EnableIRQ = (State[p++] & 1) == 1;
            Mapper_69_EnableIRQCounterDecrement = (State[p++] & 1) == 1;
            Mapper_69_IRQCounter = State[p++];
            Mapper_69_IRQCounter |= (ushort)(State[p++] << 8);
            // exitIndex = p;
        }
        void CPUClock() override
        {
            // The sunsoft FME-7 mapper chip has an IRQ counter that ticks down once per CPU cycle.
            if (Mapper_69_EnableIRQCounterDecrement)
            {
                ushort temp = Mapper_69_IRQCounter;
                Mapper_69_IRQCounter--;
                if (Mapper_69_EnableIRQ && temp < Mapper_69_IRQCounter)
                {
                    Connector_IRQPin(true); // Run an IRQ!
                }
            }
        }
    };
}