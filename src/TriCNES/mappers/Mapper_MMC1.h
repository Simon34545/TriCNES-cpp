#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_MMC1 : public Mapper
    {
        // ines Mapper 1
    public:
        byte Mapper_1_ShiftRegister = 0;
        byte Mapper_1_Control = 0x0C;    //0x8000
        byte Mapper_1_CHR0 = 0;              //0xA000
        byte Mapper_1_CHR1 = 0;              //0xC000
        byte Mapper_1_PRG = 0;               //0xE000
        bool Mapper_1_PB = false;
        void FetchPRG(ushort Address, bool Observe) override
        {
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; } else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0x8000)
            {
                notFloating = true;
                // The bank mode for MMC1:
                byte MMC1PRGROMBankMode = (byte)((Mapper_1_Control & 0b01100) >> 2);
                switch (MMC1PRGROMBankMode)
                {
                    case 0:
                    case 1:
                        {
                            // switch 32 KB at $8000, ignoring low bit of bank number
                            ushort tempo = (ushort)(Address & 0x7FFF);
                            data = Cart->PRGROM[(0x8000 * (Mapper_1_PRG & 0x0E) + tempo) % Cart->PRGROMLength];
                        }
                        break;
                    case 2:
                        // fix first bank at $8000 and switch 16 KB bank at $C000
                        if (Address >= 0xC000)
                        {
                            ushort tempo = (ushort)(Address & 0x3FFF);
                            data = Cart->PRGROM[0x4000 * (Mapper_1_PRG) + tempo];
                        }
                        else
                        {
                            ushort tempo = (ushort)(Address & 0x3FFF);
                            data = Cart->PRGROM[tempo];
                        }
                        break;
                    case 3:
                        // fix last bank at $C000 and switch 16 KB bank at $8000
                        if (Address >= 0xC000)
                        {
                            ushort tempo = (ushort)(Address & 0x3FFF);
                            data = Cart->PRGROM[Cart->PRGROMLength - 0x4000 + tempo];
                        }
                        else
                        {
                            ushort tempo = (ushort)(Address & 0x3FFF);
                            data = Cart->PRGROM[(0x4000 * (Mapper_1_PRG & 0x0F) + tempo) & (Cart->PRGROMLength - 1)];
                        }
                        break;
                }
            }
            else // if the address is < $8000
            {
                if (((Mapper_1_PRG & 0x10) == 0) && Address >= 0x6000) // if Work RAM is enabled
                {
                    data = Cart->PRGRAM[Address & 0x1FFF];
                    notFloating = true;
                }
                // else, open bus.
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
            if (Address < 0x8000) //WRAM not available on MMC1A
            {
                if (((Mapper_1_PRG & 0x10) == 0) /*&& Mapper != 1*/)
                {
                    //Battery backed RAM
                    Cart->PRGRAM[Address & 0x1FFF] = Input;
                    return;
                }
                else
                {
                    return; //do nothing
                }
            }
            else
            {   // shift the shirftRegister and add the new bit
                Mapper_1_PB = (Mapper_1_ShiftRegister & 1) == 1;
                Mapper_1_ShiftRegister >>= 1;
                Mapper_1_ShiftRegister |= (byte)((Input & 1) << 4);
            }
            if (Mapper_1_PB) // if the '1' that was initialized in bit 4 is shifted into the bus
            {
                // copy shift register to the desired internal register.
                switch (Address & 0xE000)
                {
                    case 0x8000: //control
                        Mapper_1_Control = Mapper_1_ShiftRegister;
                        break;
                    case 0xA000: //CHR0
                        Mapper_1_CHR0 = Mapper_1_ShiftRegister;
                        break;
                    case 0xC000: //CHR1
                        Mapper_1_CHR1 = Mapper_1_ShiftRegister;
                        break;
                    case 0xE000: //PRG
                        Mapper_1_PRG = Mapper_1_ShiftRegister;
                        break;
                }
                Mapper_1_ShiftRegister = 0b10000;
            }
            if ((Input & 0b10000000) != 0)
            {
                Mapper_1_ShiftRegister = 0b10000;
                Mapper_1_Control |= 0b01100;
            }
        }
        byte FetchCHR(ushort Address, bool Observe) override
        {
            // bit 4 of Mapper_1_Control controls how the pattern tables are swapped. if set, 2 banks of 4Kib. Otherwise, 1 8Kib bank
            if ((Mapper_1_Control & 0x10) != 0)
            {
                // with the MMC1 chip, you can swap out the pattern tables.
                // address < 0x1000 is the first pattern table, else, the second pattern table.
                // if the final write for the MMC1 shift register was in the $A000 - $BFFF, this updates Mapper_1_CHR0
                // if the final write for the MMC1 shift register was in the $B000 - $CFFF, this updates Mapper_1_CHR1
                if (Address < 0x1000) { return Cart->CHRROM[((Mapper_1_CHR0 & 0x1F) * 0x1000 + Address) & (Cart->CHRROMLength - 1)]; }
                else { Address &= 0xFFF; return Cart->CHRROM[((Mapper_1_CHR1 & 0x1F) * 0x1000 + Address) & (Cart->CHRROMLength - 1)]; }
            }
            else // one swappable bank that changes both pattern tables.
            {
                // this uses the value written to Mapper_1_CHR0
                return Cart->CHRROM[((Mapper_1_CHR0 & 0b11111110) * 0x2000 + Address) & (Cart->CHRROMLength - 1)];
            }
        }
        ushort MirrorNametable(ushort Address) override
        {
            switch (Mapper_1_Control & 3)
            {
                case 0: //one screen, low
                    Address &= 0x33FF;
                    break;
                case 1: //one screen, high
                    Address &= 0x33FF;
                    Address |= 0x400;
                    break;
                case 2: //vertical
                    Address &= 0x37FF; // mask away $0800
                    break;
                case 3: //horizontal
                    Address = (ushort)((Address & 0x33FF) | ((Address & 0x0800) >> 1)); // mask away $0C00, bit 10 becomes the former bit 11

                    break;
            }
            return Address;
        }
        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }
            State.push_back(Mapper_1_ShiftRegister);
            State.push_back(Mapper_1_Control);
            State.push_back(Mapper_1_CHR0);
            State.push_back(Mapper_1_CHR1);
            State.push_back(Mapper_1_ShiftRegister);
            State.push_back((byte)(Mapper_1_PB ? 1 : 0));
            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }
            Mapper_1_ShiftRegister = State[p++];
            Mapper_1_Control = State[p++];
            Mapper_1_CHR0 = State[p++];
            Mapper_1_CHR1 = State[p++];
            Mapper_1_ShiftRegister = State[p++];
            Mapper_1_PB = (State[p++] & 1) == 1;
            // exitIndex = p;
        }
    };
}