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
        void FetchPRG(ushort Address, bool Observe) override
        {
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; }
            else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

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
                                notFloating = true;
                                data = Cart->PRGRAM[Address & 0x1FFF];
                            }
                        }
                        else
                        {   //read from ROM
                            notFloating = true;
                            data = Cart->PRGROM[(Mapper_69_Bank_6 * 0x2000 + tempo) % Cart->PRGROMLength];
                        }
                    }
                    else if (Address < 0xA000)
                    {
                        notFloating = true;
                        data = Cart->PRGROM[(Mapper_69_Bank_8 * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else if (Address < 0xC000)
                    {
                        notFloating = true;
                        data = Cart->PRGROM[(Mapper_69_Bank_A * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else if (Address < 0xE000)
                    {
                        notFloating = true;
                        data = Cart->PRGROM[(Mapper_69_Bank_C * 0x2000 + tempo) % Cart->PRGROMLength];
                    }
                    else
                    {
                        notFloating = true;
                        data = Cart->PRGROM[Cart->PRGROMLength - 0x2000 + tempo];
                    }
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
                    case 13: Mapper_69_EnableIRQ = (Input & 0x1) != 0; Mapper_69_EnableIRQCounterDecrement = (Input & 0x80) != 0; Cart->Emu->IRQ_LevelDetector = false; break;
                    case 14: Mapper_69_IRQCounter = (ushort)((Mapper_69_IRQCounter & 0xFF00) | Input); break;
                    case 15: Mapper_69_IRQCounter = (ushort)((Mapper_69_IRQCounter & 0xFF) | (Input << 8)); break;
                    }
                } // else do nothing
            }
        }
        byte FetchCHR(ushort Address, bool Observe) override
        {
            if (Address < 0x400) { return Cart->CHRROM[(Mapper_69_CHR_1K0 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0x800) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K1 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0xC00) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K2 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0x1000) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K3 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0x1400) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K4 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0x1800) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K5 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else if (Address < 0x1C00) { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K6 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
            else { Address &= 0x3FF; return Cart->CHRROM[(Mapper_69_CHR_1K7 * 0x400 + Address) & (Cart->CHRROMLength - 1)]; }
        }
        ushort MirrorNametable(ushort Address) override
        {
            switch (Mapper_69_NametableMirroring)
            {
            case 0: //vertical
                Address &= 0x37FF; // mask away $0800
                break;
            case 1: //horizontal
                Address = (ushort)((Address & 0x33FF) | ((Address & 0x0800) >> 1)); // mask away $0C00, bit 10 becomes the former bit 11
                break;
            case 2: //one-screen A
                Address &= 0x33FF;
                break;
            case 3: //one-screen B
                Address &= 0x33FF;
                Address |= 0x400;
                break;
            }
            return Address;
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }
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
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }
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
                    Cart->Emu->IRQ_LevelDetector = true;
                }
            }
        }
    };
}