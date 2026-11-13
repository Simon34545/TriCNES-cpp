#include "Emulator.h"
#include "mappers.h"

#include <cstring>

namespace TriCNES
{
    Cartridge::Cartridge(std::string filepath)
        {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate); // Reads the file from the provided file path, and stores every byte into an array.
            if (!file.is_open()) {
                std::cerr << "Error opening file!" << std::endl;
                return;
            }
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            ROM = new byte[size];

            if (size > 8 && file.read((char*)ROM, size)) {
                std::cout << "Loaded ROM: " << filepath << std::endl;
            }
            else {
                std::cerr << "Error reading file!" << std::endl;
                return;
            }

            file.close();
            
            // The iNES header isn't actually part of the physical cartridge.
            // Rather, the values of the iNES header are manually added to provide extra information to emulators.
            // Info such as "what mapper chip", "how many CHR banks?" and even "how should we mirror the nametables?" are part of this header.

            MemoryMapper = (byte)(ROM[7] & 0xF0);   // Parsing the iNES header to determine what mapper chip this cartridge uses.
            MemoryMapper |= (byte)(ROM[6] >> 4);    // The upper nybble of byte 6, bitwise OR with the upper nybble of byte 7.
            SubMapper = (byte)((ROM[8] & 0xF0) >> 4);

            PRG_Size = ROM[4];  // Parsing the iNES header to determine how many kb of PRG data exists on this cartridge.
            CHR_Size = ROM[5];  // Parsing the iNES header to determine how many kb of CHR data exists on this cartridge.

            PRG_SizeMinus1 = (byte)(PRG_Size - 1); // This value is occasionally used whenever a mapper has a fixed bank from the end of the PRG data, like address $E000 in the MMC3 chip.

            UsingCHRRAM = CHR_Size == 0; // If CHR_Size == 0, this is using CHR RAM

            PRGROMLength = PRG_Size * 0x4000;
            CHRROMLength = CHR_Size * 0x2000;
            PRGRAMLength = 0x2000;
            CHRRAMLength = 0x2000;

            PRGROM = new byte[PRGROMLength]; // 0x4000 bytes of PRG ROM, multiplied by byte 4 of the iNES header.
            CHRROM = new byte[CHRROMLength]; // 0x2000 bytes of CHR ROM, multiplied by byte 5 of the iNES header.
            CHRRAM = new byte[CHRRAMLength]();            // CHR RAM always has 2 kibibytes

            NametableHorizontalMirroring = ((ROM[6] & 1) == 0); // The style in which the nametable is mirrored is part of the iNES header.
            AlternativeNametableArrangement = ((ROM[6] & 8) != 0); // Some mappers support other arrangements.
            if (AlternativeNametableArrangement)
            {
                PRGVRAM = new byte[0x800]();
                PRGVRAMLength = 0x800;
            }

            memcpy(PRGROM, &ROM[0x10], PRGROMLength); // This sets up the PRG ROM array with the values from the .nes file
            memcpy(CHRROM, &ROM[0x10 + PRGROMLength], CHRROMLength); // This sets up the CHR ROM array with the values from the .nes file

            // at this point, the ROM byte array is no longer needed, so null it to free up its memory.
            delete[] ROM;

            PRGRAM = new byte[PRGRAMLength](); // PRG RAM probably has different lengths depending on the mapper, but this emulator doesn't yet support any mappers in which that length isn't 2 kibibytes.

            Name = filepath; // For debugging, it's nice to see the file name sometimes.
            switch (MemoryMapper)
            {
                default:
                case 0: MapperChip = new Mapper_NROM(); break;
                case 1: MapperChip = new Mapper_MMC1(); break;
                case 71:
                case 2: MapperChip = new Mapper_UxROM(); break;
                case 3: MapperChip = new Mapper_CNROM(); break;
                case 4: MapperChip = new Mapper_MMC3(); break;
                case 7: MapperChip = new Mapper_AOROM(); break;
                case 9: MapperChip = new Mapper_MMC2(); break;
                case 69: MapperChip = new Mapper_FME7(); break;
            }
            MapperChip->Cart = this;
        }
    Cartridge::Cartridge(std::string filepath, std::string FDSBIOS_filepath)
        {
            std::ifstream file(FDSBIOS_filepath, std::ios::binary | std::ios::ate); // Reads the file from the provided file path, and stores every byte into an array.
            if (!file.is_open()) {
                std::cerr << "Error opening BIOS ROM file! " << FDSBIOS_filepath << std::endl;
                return;
            }
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            ROM = new byte[size];

            if (size > 8 && file.read((char*)ROM, size)) {
                std::cout << "Loaded ROM: " << FDSBIOS_filepath << std::endl;
            }
            else {
                std::cerr << "Error reading BIOS ROM file!" << std::endl;
                return;
            }

            file.close();

            FDS = new DiskDrive();
            if (!filepath.empty()) FDS->InsertDisk(filepath);
            PRGRAM = new byte[0x8000](); // The FDS has 32Kib of PRG RAM!
            CHRRAM = new byte[0x2000](); // and 8 Kib of CHR RAM.
            Name = filepath; // For debugging, it's nice to see the file name sometimes.

            MapperChip = new Mapper_FDS(ROM);
            MapperChip->Cart = this;
            FDS->Cart = this;
        }

        void Mapper::FetchPRG(ushort Address, bool Observe)
        {
            if (!Observe)
            {
                Address = Connector_ReadCPUAddressPins();
            }
            bool notFloating = false;
            byte data = 0;
            if (!Observe) { dataPinsAreNotFloating = false; } else { observedDataPinsAreNotFloating = false; }
            // Observing can happen on a different thread, so we need to ensure that observing doesn't overwrite the data bus or floating pins status.

            if (Address >= 0x8000)
            {
                data = Cart->PRGROM[Address & (Cart->PRGROMLength - 1)]; // Get the address from the ROM file. If the ROM only has $4000 bytes, this will make addresses > $BFFF mirrors of $8000 through $BFFF.
                notFloating = true;
            }
            //open bus

            if (notFloating)
            {
                EndFetchPRG(Observe, data);
            }
            return;
        }
        void Mapper::StorePRG(ushort Address, byte Input)
        {
        }
        byte Mapper::FetchCHR(ushort Address, bool Observe)
        {
            return Cart->CHRROM[Address & 0x1FFF];
        }
        void Mapper::FetchPPU()
        {
            // This will always use the upper 8 bits of the address bus | the octal latch. This Octal Latch replaces the lower 8 bits of the address bus.
            ushort Address = Connector_ReadPPUAddressPins();

            byte t = Cart->Emu->PPU_OctalLatch;
            if (Cart->Emu->SeventyTwoPinConnector[57] && Address < 0x2000) // Addresses $2000 through $3FFF do NOT read from the cartrdige.
            {
                if (Cart->UsingCHRRAM)
                {
                    t = Cart->CHRRAM[Address];
                }
                else
                {
                    //Pattern Table
                    t = Cart->MapperChip->FetchCHR(Address, false);
                }
            }
            Connector_SetUpPPUDataPins(t);
        }
        void Mapper::CheckCIRAM()
        { 
            Cart->Emu->SeventyTwoPinConnector[56] = !Cart->Emu->SeventyTwoPinConnector[57];
        }
        byte Mapper::FetchNametable()
        {
            ushort Address = (ushort)((Cart->Emu->PPU_AddressBus & 0x3F00) | Cart->Emu->PPU_OctalLatch);
            Address = Cart->MapperChip->MirrorNametable(Address);
            Address &= 0x7FF;
            return Cart->Emu->VRAM[Address];
        }
        ushort Mapper::MirrorNametable(ushort Address)
        {
            if (!Cart->NametableHorizontalMirroring)
            {
                return (ushort)(Address & 0x37FF); // mask away $0800
            }
            else // horizontal
            {
                return (ushort)((Address & 0x33FF) | ((Address & 0x0800) >> 1)); // mask away $0C00, bit 10 becomes the former bit 11
            }
        }
        std::vector<byte> Mapper::SaveMapperRegisters()
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { State.push_back(Cart->CHRRAM[i]); }
            return State;
        }
        void Mapper::LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/)
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            for (int i = 0; i < Cart->CHRRAMLength; i++) { Cart->CHRRAM[i] = State[p++]; }
            //exitIndex = p;
        }
        void Mapper::PPUClock() // runs every PPU clock. (See MMC3)
        {
        }
        void Mapper::CPUClock() // runs every CPU clock. (See Sunsoft FME-7)
        {
        }
        void Mapper::CPUClockRise() // runs every time the CPU clock rises. (See MMC3)
        {
        }

        void Mapper::FDS_ByteTransferFlag()
        {
        }
        byte Mapper::FDS_Get4025()
        {
            return 0;
        }

        void Mapper::EndFetchPRG(bool Observe, byte data)
        {
            if (!Observe)
            {
                dataPinsAreNotFloating = true;
                Connector_SetUpCPUDataPins(data);
            }
            else
            {
                observedDataPinsAreNotFloating = true;
                observedDataBus = data;
            }
        }

        void Mapper::Connector_SetUpPPUAddressPins()
        {
            Cart->Emu->SeventyTwoPinConnector[28] = (Cart->Emu->PPU_OctalLatch & 0x01) != 0; // PPU A0
            Cart->Emu->SeventyTwoPinConnector[27] = (Cart->Emu->PPU_OctalLatch & 0x02) != 0; // PPU A1
            Cart->Emu->SeventyTwoPinConnector[26] = (Cart->Emu->PPU_OctalLatch & 0x04) != 0; // PPU A2
            Cart->Emu->SeventyTwoPinConnector[25] = (Cart->Emu->PPU_OctalLatch & 0x08) != 0; // PPU A3
            Cart->Emu->SeventyTwoPinConnector[24] = (Cart->Emu->PPU_OctalLatch & 0x10) != 0; // PPU A4
            Cart->Emu->SeventyTwoPinConnector[23] = (Cart->Emu->PPU_OctalLatch & 0x20) != 0; // PPU A5
            Cart->Emu->SeventyTwoPinConnector[22] = (Cart->Emu->PPU_OctalLatch & 0x40) != 0; // PPU A6
            Cart->Emu->SeventyTwoPinConnector[58] = (Cart->Emu->PPU_OctalLatch & 0x80) != 0; // PPU A7
            Cart->Emu->SeventyTwoPinConnector[59] = (Cart->Emu->PPU_AddressBus & 0x0100) != 0; // PPU A8
            Cart->Emu->SeventyTwoPinConnector[60] = (Cart->Emu->PPU_AddressBus & 0x0200) != 0; // PPU A9
            Cart->Emu->SeventyTwoPinConnector[62] = (Cart->Emu->PPU_AddressBus & 0x0400) != 0; // PPU A10
            Cart->Emu->SeventyTwoPinConnector[61] = (Cart->Emu->PPU_AddressBus & 0x0800) != 0; // PPU A11
            Cart->Emu->SeventyTwoPinConnector[63] = (Cart->Emu->PPU_AddressBus & 0x1000) != 0; // PPU A12
            Cart->Emu->SeventyTwoPinConnector[64] = (Cart->Emu->PPU_AddressBus & 0x2000) != 0; // PPU A13
            Cart->Emu->SeventyTwoPinConnector[57] = (Cart->Emu->PPU_AddressBus & 0x2000) == 0; // PPU \A13
        }
        ushort Mapper::Connector_ReadPPUAddressPins()
        {
            ushort Address = 0;
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[64] ? 0x2000 : 0); // PPU A13
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[63] ? 0x1000 : 0); // PPU A12
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[61] ? 0x0800 : 0); // PPU A11
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[62] ? 0x0400 : 0); // PPU A10
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[60] ? 0x0200 : 0); // PPU A9
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[59] ? 0x0100 : 0); // PPU A8
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[58] ? 0x0080 : 0); // PPU A7
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[22] ? 0x0040 : 0); // PPU A6
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[23] ? 0x0020 : 0); // PPU A5
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[24] ? 0x0010 : 0); // PPU A4
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[25] ? 0x0008 : 0); // PPU A3
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[26] ? 0x0004 : 0); // PPU A2
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[27] ? 0x0002 : 0); // PPU A1
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[28] ? 0x0001 : 0); // PPU A0
            return Address;
        }
        void Mapper::Connector_SetUpPPUDataPins(byte t)
        {
            Cart->Emu->SeventyTwoPinConnector[29] = (t & 0x01) != 0; // PPU D0
            Cart->Emu->SeventyTwoPinConnector[30] = (t & 0x02) != 0; // PPU D1
            Cart->Emu->SeventyTwoPinConnector[31] = (t & 0x04) != 0; // PPU D2
            Cart->Emu->SeventyTwoPinConnector[32] = (t & 0x08) != 0; // PPU D3
            Cart->Emu->SeventyTwoPinConnector[68] = (t & 0x10) != 0; // PPU D4
            Cart->Emu->SeventyTwoPinConnector[67] = (t & 0x20) != 0; // PPU D5
            Cart->Emu->SeventyTwoPinConnector[66] = (t & 0x40) != 0; // PPU D6
            Cart->Emu->SeventyTwoPinConnector[65] = (t & 0x80) != 0; // PPU D7
        }
        byte Mapper::Connector_ReadPPUDataPins()
        {
            byte t = 0;
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[29] ? 0x01 : 0); // PPU D0
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[30] ? 0x02 : 0); // PPU D1
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[31] ? 0x04 : 0); // PPU D2
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[32] ? 0x08 : 0); // PPU D3
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[68] ? 0x10 : 0); // PPU D4
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[67] ? 0x20 : 0); // PPU D5
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[66] ? 0x40 : 0); // PPU D6
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[65] ? 0x80 : 0); // PPU D7
            return t;
        }

        void Mapper::Connector_SetUpCPUAddressPins(ushort Address) // the 2A03 bus, not the 6502 bus.
        {
            Cart->Emu->SeventyTwoPinConnector[12] = (Address & 0x01) != 0; // CPU A0
            Cart->Emu->SeventyTwoPinConnector[11] = (Address & 0x02) != 0; // CPU A1
            Cart->Emu->SeventyTwoPinConnector[10] = (Address & 0x04) != 0; // CPU A2
            Cart->Emu->SeventyTwoPinConnector[9] = (Address & 0x08) != 0; // CPU A3
            Cart->Emu->SeventyTwoPinConnector[8] = (Address & 0x10) != 0; // CPU A4
            Cart->Emu->SeventyTwoPinConnector[7] = (Address & 0x20) != 0; // CPU A5
            Cart->Emu->SeventyTwoPinConnector[6] = (Address & 0x40) != 0; // CPU A6
            Cart->Emu->SeventyTwoPinConnector[5] = (Address & 0x80) != 0; // CPU A7
            Cart->Emu->SeventyTwoPinConnector[4] = (Address & 0x0100) != 0; // CPU A8
            Cart->Emu->SeventyTwoPinConnector[3] = (Address & 0x0200) != 0; // CPU A9
            Cart->Emu->SeventyTwoPinConnector[2] = (Address & 0x0400) != 0; // CPU A10
            Cart->Emu->SeventyTwoPinConnector[1] = (Address & 0x0800) != 0; // CPU A11
            Cart->Emu->SeventyTwoPinConnector[38] = (Address & 0x1000) != 0; // CPU A12
            Cart->Emu->SeventyTwoPinConnector[39] = (Address & 0x2000) != 0; // CPU A13
            Cart->Emu->SeventyTwoPinConnector[40] = (Address & 0x4000) != 0; // CPU A14
            Cart->Emu->SeventyTwoPinConnector[49] = (Address & 0x8000) != 0; // CPU A15
        }
        ushort Mapper::Connector_ReadCPUAddressPins()
        {
            ushort Address = 0;
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[49] ? 0x8000 : 0); // CPU A0
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[40] ? 0x4000 : 0); // CPU A1
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[39] ? 0x2000 : 0); // CPU A2
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[38] ? 0x1000 : 0); // CPU A3
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[1] ? 0x0800 : 0); // CPU A4
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[2] ? 0x0400 : 0); // CPU A5
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[3] ? 0x0200 : 0); // CPU A6
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[4] ? 0x0100 : 0); // CPU A7
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[5] ? 0x0080 : 0); // CPU A8
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[6] ? 0x0040 : 0); // CPU A9
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[7] ? 0x0020 : 0); // CPU A10
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[8] ? 0x0010 : 0); // CPU A11
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[9] ? 0x0008 : 0); // CPU A12
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[10] ? 0x0004 : 0); // CPU A13
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[11] ? 0x0002 : 0); // CPU A14
            Address |= (ushort)(Cart->Emu->SeventyTwoPinConnector[12] ? 0x0001 : 0); // CPU A15
            return Address;
        }
        void Mapper::Connector_SetUpCPUDataPins(byte t)
        {
            Cart->Emu->SeventyTwoPinConnector[48] = (t & 0x01) != 0; // PPU D0
            Cart->Emu->SeventyTwoPinConnector[47] = (t & 0x02) != 0; // PPU D1
            Cart->Emu->SeventyTwoPinConnector[46] = (t & 0x04) != 0; // PPU D2
            Cart->Emu->SeventyTwoPinConnector[45] = (t & 0x08) != 0; // PPU D3
            Cart->Emu->SeventyTwoPinConnector[44] = (t & 0x10) != 0; // PPU D4
            Cart->Emu->SeventyTwoPinConnector[43] = (t & 0x20) != 0; // PPU D5
            Cart->Emu->SeventyTwoPinConnector[42] = (t & 0x40) != 0; // PPU D6
            Cart->Emu->SeventyTwoPinConnector[41] = (t & 0x80) != 0; // PPU D7
        }
        byte Mapper::Connector_ReadCPUDataPins()
        {
            byte t = 0;
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[48] ? 0x01 : 0); // PPU D0
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[47] ? 0x02 : 0); // PPU D1
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[46] ? 0x04 : 0); // PPU D2
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[45] ? 0x08 : 0); // PPU D3
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[44] ? 0x10 : 0); // PPU D4
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[43] ? 0x20 : 0); // PPU D5
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[42] ? 0x40 : 0); // PPU D6
            t |= (byte)(Cart->Emu->SeventyTwoPinConnector[41] ? 0x80 : 0); // PPU D7
            return t;
        }

        void DiskDrive::Clock()
        {
            clock++;

            switch (CurrentState)
            {
            case RamAdapterState::RUNNING:
                if (clock == 244)
                {
                    clock = 0;
                    if ((Cart->MapperChip->FDS_Get4025() & 0x2) == 0x2)
                    {
                        DiskAddress += 625; // Just doing what Neshawk does here... Basically fast forwarding until DiskAddress reaches the end?
                    }
                    else if ((Cart->MapperChip->FDS_Get4025() & 0x4) == 0x4)
                    {
                        // reading
                        byte ShiftBit = (byte)((Disk[DiskAddress] >> (DiskAddressFine)) & 1);

                        if (lookingForEndOfGap && (Cart->MapperChip->FDS_Get4025() & 0x10) == 0)
                        {
                            if (ShiftBit == 1)
                            {
                                // we found the end of the gap! :tada:
                                lookingForEndOfGap = false;
                                DiskAddressFine = 0;
                                DiskAddress++;
                            }
                            else
                            {
                                DiskAddressFine++;
                                if (DiskAddressFine == 8)
                                {
                                    DiskAddressFine = 0;
                                    DiskAddress++;
                                }
                            }
                        }
                        else
                        {
                            ShiftRegister >>= 1;
                            ShiftRegister |= (byte)(ShiftBit * 0x80);
                            DiskAddressFine++;
                            if (DiskAddressFine == 8)
                            {
                                DiskAddressFine = 0;
                                DiskAddress++;

                                ShiftRegisterLatch = ShiftRegister;
                                // disk drive is ready.
                                // raise the byte transfer flag!
                                Status_ByteTransferFlag = true;
                                Cart->MapperChip->FDS_ByteTransferFlag(); // Trigger an IRQ if $4025.7 is set.
                                if (Cart->Emu->IRQ_LevelDetector)
                                {
                                    // debugging, put breakpoint here:
                                }
                            }
                        }
                    }
                    else
                    {
                        DiskAddressFine = 0;
                    }
                    if (DiskAddress >= DiskLength)
                    {
                        CurrentState = RamAdapterState::RESET;
                    }
                }
                break;
            case RamAdapterState::RESET:
            case RamAdapterState::INSERTING:
                if (clock == 2140000)
                {
                    clock = 0;
                    DiskAddress = 0;
                    CurrentState = RamAdapterState::IDLE;
                }
                break;
            case RamAdapterState::SPINUP:
                if (clock == 4280000)
                {
                    clock = 0;
                    CurrentState = RamAdapterState::RUNNING;
                }
                break;
            case RamAdapterState::IDLE:
                clock = 0;
                break;
            }
        }


        void DiskDrive::InsertDisk(std::string filepath)
        {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate); // Reads the file from the provided file path, and stores every byte into an array.
            if (!file.is_open()) {
                std::cerr << "Error opening file!" << std::endl;
                return;
            }
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            Disk = new byte[size];

            if (size > 8 && file.read((char*)Disk, size)) {
                std::cout << "Loaded ROM: " << filepath << std::endl;
            }
            else {
                std::cerr << "Error reading file!" << std::endl;
                return;
            }

            file.close();

            DiskLength = size;

            Disk = FixFDSDiskSide(Disk);
            CurrentState = RamAdapterState::INSERTING;
        }
}