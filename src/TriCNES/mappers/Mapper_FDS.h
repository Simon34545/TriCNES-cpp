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

        void FetchCPU() override
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[71]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();
            ushort Address = CPU_AddressIn;

            if (CPU_AddressIn >= 0xE000)
            {
                // read from the FDS BIOS
                CPU_DataOut = FDS_BIOS[Address & 0x1FFF];
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            else if (Address >= 0x6000)
            {
                // read from the FDS PRG RAM
                CPU_DataOut = Cart->PRGRAM[Address - 0x6000];
                Connector_SetUpCPUDataPins(CPU_DataOut);
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
                    CPU_DataOut = 0;
                    CPU_DataOut |= (byte)((FDS_4025_Control >> 3) & 1); // 4030.3 = 4025.3
                    CPU_DataOut |= (byte)((Cart->FDS->DiskAddress >= Cart->FDS->DiskLength) ? 0x40 : 0); // 4030.6 = End of Disk
                    CPU_DataOut |= (byte)(Cart->FDS->Status_ByteTransferFlag ? 0x80 : 0); // 4030.7 = Byte Transfer Flag
                    Connector_SetUpCPUDataPins(CPU_DataOut);
                }
                break;
                case 1:
                {
                    // Disk Data Input ($4031)
                    CPU_DataOut = Cart->FDS->ShiftRegisterLatch;
                    Connector_SetUpCPUDataPins(CPU_DataOut);
                    Cart->FDS->Status_ByteTransferFlag = false;
                    Connector_IRQPin(false); //acknowledge the IRQ
                }
                break;
                case 2:
                {
                    // Disk Drive Status ($4032)
                    CPU_DataOut = 0;
                    if (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::INSERTING)
                    {
                        CPU_DataOut |= 1;
                    }
                    if (!(((FDS_4025_Control & 2) == 0) && (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::RUNNING || Cart->FDS->CurrentState == DiskDrive::RamAdapterState::IDLE)))
                    {
                        CPU_DataOut |= 2;
                    }
                    Connector_SetUpCPUDataPins(CPU_DataOut);
                }
                break;
                case 3:
                {
                    // External Connector Input ($4033)
                    CPU_DataOut = 0x80; // The battery is good.
                    Connector_SetUpCPUDataPins(CPU_DataOut);
                }
                break;
                }
            }

            return;
        }
        int FetchPatternAddress(ushort Address) override
        {
            return Address;
        }
        void StoreCPU(ushort Address, byte Input) override
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
                        Connector_IRQPin(false); //acknowledge the IRQ
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
        byte SnoopCPU(ushort Address) override // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0xE000)
            {
                // read from the FDS BIOS
                return FDS_BIOS[Address & 0x1FFF];
            }
            else if (Address >= 0x6000)
            {
                // read from the FDS PRG RAM
                return Cart->PRGRAM[Address - 0x6000];
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
                    byte t = 0;
                    t |= (byte)((FDS_4025_Control >> 3) & 1); // 4030.3 = 4025.3
                    t |= (byte)((Cart->FDS->DiskAddress >= Cart->FDS->DiskLength) ? 0x40 : 0); // 4030.6 = End of Disk
                    t |= (byte)(Cart->FDS->Status_ByteTransferFlag ? 0x80 : 0); // 4030.7 = Byte Transfer Flag
                    return t;
                }
                break;
                case 1:
                {
                    // Disk Data Input ($4031)
                    return Cart->FDS->ShiftRegisterLatch;
                }
                break;
                case 2:
                {
                    // Disk Drive Status ($4032)
                    byte t = 0;
                    if (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::INSERTING)
                    {
                        t |= 1;
                    }
                    if (!(((FDS_4025_Control & 2) == 0) && (Cart->FDS->CurrentState == DiskDrive::RamAdapterState::RUNNING || Cart->FDS->CurrentState == DiskDrive::RamAdapterState::IDLE)))
                    {
                        t |= 2;
                    }
                    return t;
                }
                break;
                case 3:
                {
                    // External Connector Input ($4033)
                    return 0x80; // The battery is good.
                }
                break;
                }
            }
            return Cart->Emu->dataBus;
        }
        void Connector_CheckCIRAM() override
        {
            if (TiltingCart)
            {
                if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = Cart->Emu->SeventyTwoPinConnector[57]; }
                if (((FDS_4025_Control >> 3) & 1) == 1) //horizontal
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
                Cart->Emu->SeventyTwoPinConnector[21] = (((FDS_4025_Control >> 3) & 1) == 1) ? ((Cart->Emu->PPU_AddressBus & 0x800) != 0) : ((Cart->Emu->PPU_AddressBus & 0x400) != 0);
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
                Addr |= (ushort)(((((FDS_4025_Control >> 3) & 1) == 1) ? ((Address & 0x800) != 0) : ((Address & 0x400) != 0)) ? 0x400 : 0);
                return Cart->Emu->VRAM[Addr];
            }
        }
        void FDS_ByteTransferFlag() override
        {
            if ((FDS_4025_Control & 0x80) != 0)
            {
                Connector_IRQPin(true); // Run an IRQ!
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
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { State.push_back(Cart->CHRROM[i]); }
            }

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
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { Cart->CHRROM[i] = State[p++]; }
            }

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