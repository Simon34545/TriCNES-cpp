#include "../Emulator.h"

namespace TriCNES
{
    class Mapper_FDS : public Mapper
    {
        // The Famicom Disk System
    public:
        byte* FDS_BIOS = nullptr;

        byte FDS_4023_IOEnable = 0;
        byte FDS_4025_Control = 0;

        Mapper_FDS(byte* fds_bios)
        {
            FDS_BIOS = fds_bios;
        }

        void FetchPRG(ushort Address, bool Observe) override
        {
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; }
            else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0xE000)
            {
                // read from the FDS BIOS
                notFloating = true;
                data = FDS_BIOS[Address & 0x1FFF];
            }
            else if (Address >= 0x6000)
            {
                // read from the FDS PRG RAM
                notFloating = true;
                data = Cart->PRGRAM[Address - 0x6000];
            }
            else if (Address >= 4030 && Address <= 0x403F)
            {
                // Read from the FDS Registers
                Address &= 0xF;
                switch (Address)
                {
                default: break;
                case 0:
                {
                    // FDS Status ($4030)
                    notFloating = true;
                    data = 0;

                    data |= (byte)((FDS_4025_Control >> 3) & 1); // 4030.3 = 4025.3
                        
                    data |= (byte)((Cart->FDS->DiskAddress >= Cart->FDS->DiskLength) ? 0x40 : 0); // 4030.6 = End of Disk


                    data |= (byte)(Cart->FDS->Status_ByteTransferFlag ? 0x80 : 0); // 4030.7 = Byte Transfer Flag


                }
                break;
                case 1:
                {
                    // Disk Data Input ($4031)
                    notFloating = true;
                    data = Cart->FDS->ShiftRegisterLatch;
                    Cart->FDS->Status_ByteTransferFlag = false;
                    Cart->Emu->IRQ_LevelDetector = false; //acknowledge the IRQ
                }
                break;
                case 2:
                {
                    // Disk Drive Status ($4032)
                    notFloating = true;
                    data = 0;
                    if (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::INSERTING)
                    {
                        data |= 1;
                    }
                    if (!(((FDS_4025_Control & 2) == 0) && (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::RUNNING || Cart->FDS->CurrentState == DiskDrive::RamAdapterState::IDLE)))
                    {
                        data |= 2;
                    }
                }
                break;
                case 3:
                {
                    // External Connector Input ($4033)
                    notFloating = true;
                    data = 0x80; // The battery is good.
                }
                break;
                }
            }

            if (notFloating)
            {
                EndFetchPRG(Observe, data);
            }
            return;
        }
        byte FetchCHR(ushort Address, bool Observe) override
        {
            return Cart->CHRRAM[Address];
        }
        void StorePRG(ushort Address, byte Input) override
        {
            if (Address >= 0x6000 && Address < 0xE000)
            {
                Cart->PRGRAM[Address - 0x6000] = Input;
                return;
            }
            else if (Address > 0x401F)
            {
                ushort tempo = (ushort)(Address & 0x40FF);
                switch (tempo)
                {
                case 0x4023:
                    FDS_4023_IOEnable = Input;
                    if ((FDS_4023_IOEnable & 1) == 0)
                    {
                        // Disable disk I/O registers
                        Cart->Emu->IRQ_LevelDetector = false; //acknowledge the IRQ
                        FDS_4025_Control &= 0xF3;
                        FDS_4025_Control |= 6;
                    }
                    return;
                case 0x4024:
                    Cart->FDS->Status_ByteTransferFlag = false;
                    return;
                case 0x4025:
                    if ((FDS_4025_Control & 0x40) == 0 && (Input & 0x40) != 0)
                    {
                        Cart->FDS->lookingForEndOfGap = true;
                    }
                    FDS_4025_Control = Input;
                    if ((Input & 1) != 0)
                    {
                        if (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::IDLE)
                        {
                            Cart->FDS->CurrentState = DiskDrive::RamAdapterState::SPINUP;
                        }
                    }
                    if ((FDS_4025_Control & 2) != 0)
                    {
                        // debugging: put breakpoint here
                    }
                    if ((FDS_4025_Control & 2) == 0)
                    {
                        // debugging: put breakpoint here
                    }
                    return;
                }
            }
        }
        ushort MirrorNametable(ushort Address) override
        {
            if (((FDS_4025_Control >> 3) & 1) == 1) //horizontal
            {
                return (ushort)((Address & 0x33FF) | ((Address & 0x0800) >> 1)); // mask away $0C00, bit 10 becomes the former bit 11
            }
            else //vertical
            {
                return (ushort)(Address & 0x37FF); // mask away $0800
            }

            return Address;
        }
        void FDS_ByteTransferFlag() override
        {
            if ((FDS_4025_Control & 0x80) != 0)
            {
                Cart->Emu->IRQ_LevelDetector = true;
            }
        }
        byte FDS_Get4025() override
        {
            return FDS_4025_Control;
        }

        std::vector<byte> SaveMapperRegisters() override
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }

            State.push_back(FDS_4025_Control);
            State.push_back((byte)Cart->FDS->clock);
            State.push_back((byte)(Cart->FDS->clock >> 8));
            State.push_back((byte)(Cart->FDS->clock >> 16));
            State.push_back((byte)(Cart->FDS->clock >> 24));
            State.push_back((byte)Cart->FDS->CurrentState);
            State.push_back(Cart->FDS->ShiftRegister);
            State.push_back(Cart->FDS->ShiftRegisterLatch);
            State.push_back((byte)Cart->FDS->DiskAddress);
            State.push_back((byte)(Cart->FDS->DiskAddress >> 8));
            State.push_back((byte)(Cart->FDS->DiskAddress >> 16));
            State.push_back((byte)(Cart->FDS->DiskAddress >> 24));
            State.push_back(Cart->FDS->DiskAddressFine);
            State.push_back((byte)(Cart->FDS->Status_ByteTransferFlag ? 1 : 0));
            State.push_back((byte)(Cart->FDS->lookingForEndOfGap ? 1 : 0));

            return State;
        }
        void LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/) override
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }

            FDS_4025_Control = State[p++];
            Cart->FDS->clock = State[p++];
            Cart->FDS->clock |= (ushort)(State[p++] << 8);
            Cart->FDS->clock |= (ushort)(State[p++] << 16);
            Cart->FDS->clock |= (ushort)(State[p++] << 24);
            Cart->FDS->CurrentState = (DiskDrive::RamAdapterState)State[p++];
            Cart->FDS->ShiftRegister = State[p++];
            Cart->FDS->ShiftRegisterLatch = State[p++];
            Cart->FDS->DiskAddress = State[p++];
            Cart->FDS->DiskAddress |= (ushort)(State[p++] << 8);
            Cart->FDS->DiskAddress |= (ushort)(State[p++] << 16);
            Cart->FDS->DiskAddress |= (ushort)(State[p++] << 24);
            Cart->FDS->DiskAddressFine = State[p++];
            Cart->FDS->Status_ByteTransferFlag = State[p++] == 1;
            Cart->FDS->lookingForEndOfGap = State[p++] == 1;

            // exitIndex = p;
        };
    };
}