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
            PRGRAMLength = 0x2000;

            CHRROMLength = 0;

            if (!UsingCHRRAM)
            {
                CHRROMLength = CHR_Size * 0x2000; // 0x2000 bytes of CHR ROM, multiplied by byte 5 of the iNES header.
            }
            else
            {
                if (ROM[11] == 0)
                {
                    CHRROMLength = 0x2000; // Default to 0x2000 bytes of CHR RAM.
                }
                else
                {
                    CHRROMLength = 64 << ROM[11]; // 0x2000 bytes of CHR ROM, multiplied by byte 5 of the iNES header.
                }
            }

            PRGROM = new byte[PRGROMLength]; // 0x4000 bytes of PRG ROM, multiplied by byte 4 of the iNES header.
            CHRROM = new byte[CHRROMLength];

            NametableHorizontalMirroring = ((ROM[6] & 1) == 0); // The style in which the nametable is mirrored is part of the iNES header.
            AlternativeNametableArrangement = ((ROM[6] & 8) != 0); // Some mappers support other arrangements.
            if (AlternativeNametableArrangement)
            {
                PRGVRAM = new byte[0x800]();
                PRGVRAMLength = 0x800;
            }

            memcpy(PRGROM, &ROM[0x10], PRGROMLength); // This sets up the PRG ROM array with the values from the .nes file
            
            if (!UsingCHRRAM)
            {
                memcpy(CHRROM, &ROM[0x10 + PRGROMLength], CHRROMLength); // This sets up the CHR ROM array with the values from the .nes file
            }

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
            CHRROM = new byte[0x2000](); // and 8 Kib of CHR RAM.
            UsingCHRRAM = true;
            Name = filepath; // For debugging, it's nice to see the file name sometimes.

            MapperChip = new Mapper_FDS(ROM);
            MapperChip->Cart = this;
            FDS->Cart = this;
        }
        void Mapper::FetchCPU()
        {
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[75]) || Cart->Emu->ConnectorPinFloating[35]) { return; } // If the cartridge is disconnected from power or ground, it cannot do anything.
            Connector_ReadCPUAddressPins();

            if (!Cart->Emu->SeventyTwoPinConnector[49]) // CPU /A15 + /M2
            {
                CPU_DataOut = Cart->PRGROM[CPU_AddressIn & (Cart->PRGROMLength - 1)]; // Get the address from the ROM file. If the ROM only has $4000 bytes, this will make addresses > $BFFF mirrors of $8000 through $BFFF.
                Connector_SetUpCPUDataPins(CPU_DataOut);
            }
            return;
        }


        byte Mapper::SnoopCPU(ushort Address) // For debug purposes. It's a bit clunky.
        {
            if (Address >= 0x8000)
            {
                return Cart->PRGROM[Address & (Cart->PRGROMLength - 1)];
            }
            return Cart->Emu->dataBus;
        }
        byte Mapper::SnoopPPU(ushort Address) // For debug purposes. It's a bit clunky having to set this up for every mapper with a non-NROM CIRAM setup.
        {
            if (Address < 0x2000)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                return Cart->CHRROM[CHR_Address];
            }
            else
            {
                ushort Addr = (ushort)(Address & 0x3FF);
                Addr |= (ushort)((Cart->NametableHorizontalMirroring ? ((Address & 0x800) != 0) : ((Address & 0x400) != 0)) ? 0x400 : 0);
                return Cart->Emu->VRAM[Addr];
            }
        }
        void Mapper::StoreCPU(ushort Address, byte Input)
        {
        }
        int Mapper::FetchPatternAddress(ushort Address)
        {
            return Address & 0x1FFF;
        }
        void Mapper::AccessPPU()
        {
            Connector_ReadPPUAddressPins();
            ushort Address = PPU_AddressIn;
            if (!Cart->Emu->SeventyTwoPinConnector[64]) // (If PPU A13 is set, we don't do anything on the cartridge)
            {
                int CHR_Address = Cart->MapperChip->FetchPatternAddress(Address);
                PPU_DataIn = Connector_ReadPPUDataPins(PPU_DataIn);
                if (!Cart->Emu->SeventyTwoPinConnector[20]) // Reads
                {
                    PPU_DataOut = Cart->CHRROM[CHR_Address];
                    Connector_SetUpPPUDataPins(PPU_DataOut);
                }
                if (!Cart->Emu->SeventyTwoPinConnector[55] && Cart->UsingCHRRAM) // Writes
                {
                    Cart->CHRROM[CHR_Address] = PPU_DataIn;
                }
            }
            else if (Cart->AlternativeNametableArrangement && Cart->Emu->SeventyTwoPinConnector[61]) // Unless we have an extra Nametable
            {
                Address &= 0x7FF; // I don't believe there are any carts that have an alternate nametable arrangement that aren't set up this way.
                PPU_DataIn = Connector_ReadPPUDataPins(PPU_DataIn);
                if (!Cart->Emu->SeventyTwoPinConnector[20]) // Reads
                {
                    PPU_DataOut = Cart->PRGVRAM[Address];
                    Connector_SetUpPPUDataPins(PPU_DataOut);
                }
                if (!Cart->Emu->SeventyTwoPinConnector[55]) // Writes
                {
                    Cart->PRGVRAM[Address] = PPU_DataIn;
                }
            }
        }
        std::vector<byte> Mapper::SaveMapperRegisters()
        {
            std::vector<byte> State;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { State.push_back(Cart->PRGRAM[i]); }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { State.push_back(Cart->CHRROM[i]); }
            }
            return State;
        }
        void Mapper::LoadMapperRegisters(std::vector<byte> State, int startIndex/*, int &exitIndex*/)
        {
            int p = startIndex;
            for (int i = 0; i < Cart->PRGRAMLength; i++) { Cart->PRGRAM[i] = State[p++]; }
            if (Cart->UsingCHRRAM)
            {
                for (int i = 0; i < Cart->CHRROMLength; i++) { Cart->CHRROM[i] = State[p++]; }
            }
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

        bool Mapper::CheckCIC()
        {
            // I'm not actually emulating the CIC internals. (... yet.)
            // Instead, I'll simply return true so long as the cartridge has power.
            if ((Cart->Emu->ConnectorPinFloating[0] && Cart->Emu->ConnectorPinFloating[75]) || Cart->Emu->ConnectorPinFloating[35]) { return false; } // If the cartridge is disconnected from power or ground, it cannot do anything, so the CIC check fails.
            if (Cart->Emu->ConnectorPinFloating[33] || Cart->Emu->ConnectorPinFloating[34] || Cart->Emu->ConnectorPinFloating[70]) { return false; } // If these pins for the CIC chip get disconnected, the CIC chip cannot match the console's CIC.
            return true;
        }

        void Mapper::Connector_SetUpPPUAddressPins()
        {
            if (!Cart->Emu->ConnectorPinFloating[28]) { Cart->Emu->SeventyTwoPinConnector[28] = (Cart->Emu->PPU_OctalLatch & 0x01) != 0; } // PPU A0
            if (!Cart->Emu->ConnectorPinFloating[27]) { Cart->Emu->SeventyTwoPinConnector[27] = (Cart->Emu->PPU_OctalLatch & 0x02) != 0; } // PPU A1
            if (!Cart->Emu->ConnectorPinFloating[26]) { Cart->Emu->SeventyTwoPinConnector[26] = (Cart->Emu->PPU_OctalLatch & 0x04) != 0; } // PPU A2
            if (!Cart->Emu->ConnectorPinFloating[25]) { Cart->Emu->SeventyTwoPinConnector[25] = (Cart->Emu->PPU_OctalLatch & 0x08) != 0; } // PPU A3
            if (!Cart->Emu->ConnectorPinFloating[24]) { Cart->Emu->SeventyTwoPinConnector[24] = (Cart->Emu->PPU_OctalLatch & 0x10) != 0; } // PPU A4
            if (!Cart->Emu->ConnectorPinFloating[23]) { Cart->Emu->SeventyTwoPinConnector[23] = (Cart->Emu->PPU_OctalLatch & 0x20) != 0; } // PPU A5
            if (!Cart->Emu->ConnectorPinFloating[22]) { Cart->Emu->SeventyTwoPinConnector[22] = (Cart->Emu->PPU_OctalLatch & 0x40) != 0; } // PPU A6
            if (!Cart->Emu->ConnectorPinFloating[58]) { Cart->Emu->SeventyTwoPinConnector[58] = (Cart->Emu->PPU_OctalLatch & 0x80) != 0; } // PPU A7
            if (!Cart->Emu->ConnectorPinFloating[59]) { Cart->Emu->SeventyTwoPinConnector[59] = (Cart->Emu->PPU_AddressBus & 0x0100) != 0; } // PPU A8
            if (!Cart->Emu->ConnectorPinFloating[60]) { Cart->Emu->SeventyTwoPinConnector[60] = (Cart->Emu->PPU_AddressBus & 0x0200) != 0; } // PPU A9
            if (!Cart->Emu->ConnectorPinFloating[62]) { Cart->Emu->SeventyTwoPinConnector[62] = (Cart->Emu->PPU_AddressBus & 0x0400) != 0; } // PPU A10
            if (!Cart->Emu->ConnectorPinFloating[61]) { Cart->Emu->SeventyTwoPinConnector[61] = (Cart->Emu->PPU_AddressBus & 0x0800) != 0; } // PPU A11
            if (!Cart->Emu->ConnectorPinFloating[63]) { Cart->Emu->SeventyTwoPinConnector[63] = (Cart->Emu->PPU_AddressBus & 0x1000) != 0; } // PPU A12
            if (!Cart->Emu->ConnectorPinFloating[64]) { Cart->Emu->SeventyTwoPinConnector[64] = (Cart->Emu->PPU_AddressBus & 0x2000) != 0; } // PPU A13
            if (!Cart->Emu->ConnectorPinFloating[57]) { Cart->Emu->SeventyTwoPinConnector[57] = (Cart->Emu->PPU_AddressBus & 0x2000) == 0; } // PPU \A13
        }
        void Mapper::Connector_ReadPPUAddressPins()
        {
            // This can only be done by the cartridge.
            if (!Cart->Emu->ConnectorPinFloating[64]) { PPU_AddressIn &= 0x1FFF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[64] ? 0x2000 : 0); } // PPU A13
            if (!Cart->Emu->ConnectorPinFloating[63]) { PPU_AddressIn &= 0x2FFF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[63] ? 0x1000 : 0); } // PPU A12
            if (!Cart->Emu->ConnectorPinFloating[61]) { PPU_AddressIn &= 0x37FF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[61] ? 0x0800 : 0); } // PPU A11
            if (!Cart->Emu->ConnectorPinFloating[62]) { PPU_AddressIn &= 0x3BFF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[62] ? 0x0400 : 0); } // PPU A10
            if (!Cart->Emu->ConnectorPinFloating[60]) { PPU_AddressIn &= 0x3DFF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[60] ? 0x0200 : 0); } // PPU A9
            if (!Cart->Emu->ConnectorPinFloating[59]) { PPU_AddressIn &= 0x3EFF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[59] ? 0x0100 : 0); } // PPU A8
            if (!Cart->Emu->ConnectorPinFloating[58]) { PPU_AddressIn &= 0x3F7F; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[58] ? 0x0080 : 0); } // PPU A7
            if (!Cart->Emu->ConnectorPinFloating[22]) { PPU_AddressIn &= 0x3FBF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[22] ? 0x0040 : 0); } // PPU A6
            if (!Cart->Emu->ConnectorPinFloating[23]) { PPU_AddressIn &= 0x3FDF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[23] ? 0x0020 : 0); } // PPU A5
            if (!Cart->Emu->ConnectorPinFloating[24]) { PPU_AddressIn &= 0x3FEF; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[24] ? 0x0010 : 0); } // PPU A4
            if (!Cart->Emu->ConnectorPinFloating[25]) { PPU_AddressIn &= 0x3FF7; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[25] ? 0x0008 : 0); } // PPU A3
            if (!Cart->Emu->ConnectorPinFloating[26]) { PPU_AddressIn &= 0x3FFB; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[26] ? 0x0004 : 0); } // PPU A2
            if (!Cart->Emu->ConnectorPinFloating[27]) { PPU_AddressIn &= 0x3FFD; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[27] ? 0x0002 : 0); } // PPU A1
            if (!Cart->Emu->ConnectorPinFloating[28]) { PPU_AddressIn &= 0x3FFE; PPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[28] ? 0x0001 : 0); } // PPU A0
        }
        void Mapper::Connector_SetUpPPUDataPins(byte t)
        {
            if (!Cart->Emu->ConnectorPinFloating[29]) { Cart->Emu->SeventyTwoPinConnector[29] = (t & 0x01) != 0; } // PPU D0
            if (!Cart->Emu->ConnectorPinFloating[30]) { Cart->Emu->SeventyTwoPinConnector[30] = (t & 0x02) != 0; } // PPU D1
            if (!Cart->Emu->ConnectorPinFloating[31]) { Cart->Emu->SeventyTwoPinConnector[31] = (t & 0x04) != 0; } // PPU D2
            if (!Cart->Emu->ConnectorPinFloating[32]) { Cart->Emu->SeventyTwoPinConnector[32] = (t & 0x08) != 0; } // PPU D3
            if (!Cart->Emu->ConnectorPinFloating[68]) { Cart->Emu->SeventyTwoPinConnector[68] = (t & 0x10) != 0; } // PPU D4
            if (!Cart->Emu->ConnectorPinFloating[67]) { Cart->Emu->SeventyTwoPinConnector[67] = (t & 0x20) != 0; } // PPU D5
            if (!Cart->Emu->ConnectorPinFloating[66]) { Cart->Emu->SeventyTwoPinConnector[66] = (t & 0x40) != 0; } // PPU D6
            if (!Cart->Emu->ConnectorPinFloating[65]) { Cart->Emu->SeventyTwoPinConnector[65] = (t & 0x80) != 0; } // PPU D7
        }
        byte Mapper::Connector_ReadPPUDataPins(byte bus)
        {
            byte t = bus;
            if (!Cart->Emu->ConnectorPinFloating[29]) { t &= 0xFE; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[29] ? 0x01 : 0); } // PPU D0
            if (!Cart->Emu->ConnectorPinFloating[30]) { t &= 0xFD; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[30] ? 0x02 : 0); } // PPU D1
            if (!Cart->Emu->ConnectorPinFloating[31]) { t &= 0xFB; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[31] ? 0x04 : 0); } // PPU D2
            if (!Cart->Emu->ConnectorPinFloating[32]) { t &= 0xF7; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[32] ? 0x08 : 0); } // PPU D3
            if (!Cart->Emu->ConnectorPinFloating[68]) { t &= 0xEF; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[68] ? 0x10 : 0); } // PPU D4
            if (!Cart->Emu->ConnectorPinFloating[67]) { t &= 0xDF; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[67] ? 0x20 : 0); } // PPU D5
            if (!Cart->Emu->ConnectorPinFloating[66]) { t &= 0xBF; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[66] ? 0x40 : 0); } // PPU D6
            if (!Cart->Emu->ConnectorPinFloating[65]) { t &= 0x7F; t |= (byte)(Cart->Emu->SeventyTwoPinConnector[65] ? 0x80 : 0); } // PPU D7
            return t;
        }

        void Mapper::Connector_SetUpCPUAddressPins(ushort Address) // the 2A03 bus, not the 6502 bus.
        {
            if (!Cart->Emu->ConnectorPinFloating[12]) { Cart->Emu->SeventyTwoPinConnector[12] = (Address & 0x01) != 0; } // CPU A0
            if (!Cart->Emu->ConnectorPinFloating[11]) { Cart->Emu->SeventyTwoPinConnector[11] = (Address & 0x02) != 0; } // CPU A1
            if (!Cart->Emu->ConnectorPinFloating[10]) { Cart->Emu->SeventyTwoPinConnector[10] = (Address & 0x04) != 0; } // CPU A2
            if (!Cart->Emu->ConnectorPinFloating[9]) { Cart->Emu->SeventyTwoPinConnector[9] = (Address & 0x08) != 0; } // CPU A3
            if (!Cart->Emu->ConnectorPinFloating[8]) { Cart->Emu->SeventyTwoPinConnector[8] = (Address & 0x10) != 0; } // CPU A4
            if (!Cart->Emu->ConnectorPinFloating[7]) { Cart->Emu->SeventyTwoPinConnector[7] = (Address & 0x20) != 0; } // CPU A5
            if (!Cart->Emu->ConnectorPinFloating[6]) { Cart->Emu->SeventyTwoPinConnector[6] = (Address & 0x40) != 0; } // CPU A6
            if (!Cart->Emu->ConnectorPinFloating[5]) { Cart->Emu->SeventyTwoPinConnector[5] = (Address & 0x80) != 0; } // CPU A7
            if (!Cart->Emu->ConnectorPinFloating[4]) { Cart->Emu->SeventyTwoPinConnector[4] = (Address & 0x0100) != 0; } // CPU A8
            if (!Cart->Emu->ConnectorPinFloating[3]) { Cart->Emu->SeventyTwoPinConnector[3] = (Address & 0x0200) != 0; } // CPU A9
            if (!Cart->Emu->ConnectorPinFloating[2]) { Cart->Emu->SeventyTwoPinConnector[2] = (Address & 0x0400) != 0; } // CPU A10
            if (!Cart->Emu->ConnectorPinFloating[1]) { Cart->Emu->SeventyTwoPinConnector[1] = (Address & 0x0800) != 0; } // CPU A11
            if (!Cart->Emu->ConnectorPinFloating[38]) { Cart->Emu->SeventyTwoPinConnector[38] = (Address & 0x1000) != 0; } // CPU A12
            if (!Cart->Emu->ConnectorPinFloating[39]) { Cart->Emu->SeventyTwoPinConnector[39] = (Address & 0x2000) != 0; } // CPU A13
            if (!Cart->Emu->ConnectorPinFloating[40]) { Cart->Emu->SeventyTwoPinConnector[40] = (Address & 0x4000) != 0; } // CPU A14
            if (!Cart->Emu->ConnectorPinFloating[49]) { Cart->Emu->SeventyTwoPinConnector[49] = (Address & 0x8000) == 0; } // CPU /A15
        }
        void Mapper::Connector_ReadCPUAddressPins()
        {
            // This can only be done by the cartridge.
            if (!Cart->Emu->ConnectorPinFloating[49]) { CPU_AddressIn &= 0x7FFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[49] ? 0 : 0x8000); } // CPU /A15
            if (!Cart->Emu->ConnectorPinFloating[40]) { CPU_AddressIn &= 0xBFFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[40] ? 0x4000 : 0); } // CPU A14
            if (!Cart->Emu->ConnectorPinFloating[39]) { CPU_AddressIn &= 0xDFFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[39] ? 0x2000 : 0); } // CPU A13
            if (!Cart->Emu->ConnectorPinFloating[38]) { CPU_AddressIn &= 0xEFFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[38] ? 0x1000 : 0); } // CPU A12
            if (!Cart->Emu->ConnectorPinFloating[1]) { CPU_AddressIn &= 0xF7FF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[1] ? 0x0800 : 0); } // CPU A11
            if (!Cart->Emu->ConnectorPinFloating[2]) { CPU_AddressIn &= 0xFBFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[2] ? 0x0400 : 0); } // CPU A10
            if (!Cart->Emu->ConnectorPinFloating[3]) { CPU_AddressIn &= 0xFDFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[3] ? 0x0200 : 0); } // CPU A9
            if (!Cart->Emu->ConnectorPinFloating[4]) { CPU_AddressIn &= 0xFEFF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[4] ? 0x0100 : 0); } // CPU A8
            if (!Cart->Emu->ConnectorPinFloating[5]) { CPU_AddressIn &= 0xFF7F; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[5] ? 0x0080 : 0); } // CPU A7
            if (!Cart->Emu->ConnectorPinFloating[6]) { CPU_AddressIn &= 0xFFBF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[6] ? 0x0040 : 0); } // CPU A6
            if (!Cart->Emu->ConnectorPinFloating[7]) { CPU_AddressIn &= 0xFFDF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[7] ? 0x0020 : 0); } // CPU A5
            if (!Cart->Emu->ConnectorPinFloating[8]) { CPU_AddressIn &= 0xFFEF; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[8] ? 0x0010 : 0); } // CPU A4
            if (!Cart->Emu->ConnectorPinFloating[9]) { CPU_AddressIn &= 0xFFF7; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[9] ? 0x0008 : 0); } // CPU A3
            if (!Cart->Emu->ConnectorPinFloating[10]) { CPU_AddressIn &= 0xFFFB; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[10] ? 0x0004 : 0); } // CPU A2
            if (!Cart->Emu->ConnectorPinFloating[11]) { CPU_AddressIn &= 0xFFFD; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[11] ? 0x0002 : 0); } // CPU A1
            if (!Cart->Emu->ConnectorPinFloating[12]) { CPU_AddressIn &= 0xFFFE; CPU_AddressIn |= (ushort)(Cart->Emu->SeventyTwoPinConnector[12] ? 0x0001 : 0); } // CPU A0
        }
        void Mapper::Connector_SetUpCPUDataPins(byte t)
        {
            if (!Cart->Emu->ConnectorPinFloating[48]) { Cart->Emu->SeventyTwoPinConnector[48] = (t & 0x01) != 0; } // CPU D0
            if (!Cart->Emu->ConnectorPinFloating[47]) { Cart->Emu->SeventyTwoPinConnector[47] = (t & 0x02) != 0; } // CPU D1
            if (!Cart->Emu->ConnectorPinFloating[46]) { Cart->Emu->SeventyTwoPinConnector[46] = (t & 0x04) != 0; } // CPU D2
            if (!Cart->Emu->ConnectorPinFloating[45]) { Cart->Emu->SeventyTwoPinConnector[45] = (t & 0x08) != 0; } // CPU D3
            if (!Cart->Emu->ConnectorPinFloating[44]) { Cart->Emu->SeventyTwoPinConnector[44] = (t & 0x10) != 0; } // CPU D4
            if (!Cart->Emu->ConnectorPinFloating[43]) { Cart->Emu->SeventyTwoPinConnector[43] = (t & 0x20) != 0; } // CPU D5
            if (!Cart->Emu->ConnectorPinFloating[42]) { Cart->Emu->SeventyTwoPinConnector[42] = (t & 0x40) != 0; } // CPU D6
            if (!Cart->Emu->ConnectorPinFloating[41]) { Cart->Emu->SeventyTwoPinConnector[41] = (t & 0x80) != 0; } // CPU D7
        }
        byte Mapper::Connector_ReadCPUDataPins()
        {
            byte t = 0;
            if (!Cart->Emu->ConnectorPinFloating[48]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[48] ? 0x01 : 0); } // CPU D0
            if (!Cart->Emu->ConnectorPinFloating[47]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[47] ? 0x02 : 0); } // CPU D1
            if (!Cart->Emu->ConnectorPinFloating[46]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[46] ? 0x04 : 0); } // CPU D2
            if (!Cart->Emu->ConnectorPinFloating[45]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[45] ? 0x08 : 0); } // CPU D3
            if (!Cart->Emu->ConnectorPinFloating[44]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[44] ? 0x10 : 0); } // CPU D4
            if (!Cart->Emu->ConnectorPinFloating[43]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[43] ? 0x20 : 0); } // CPU D5
            if (!Cart->Emu->ConnectorPinFloating[42]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[42] ? 0x40 : 0); } // CPU D6
            if (!Cart->Emu->ConnectorPinFloating[41]) { t |= (byte)(Cart->Emu->SeventyTwoPinConnector[41] ? 0x80 : 0); } // CPU D7
            return t;
        }
        void Mapper::Connector_CheckCIRAM()
        {
            if (!Cart->Emu->ConnectorPinFloating[56]) { Cart->Emu->SeventyTwoPinConnector[56] = !Cart->Emu->SeventyTwoPinConnector[57]; }
            if (!Cart->Emu->ConnectorPinFloating[21]) { Cart->Emu->SeventyTwoPinConnector[21] = Cart->NametableHorizontalMirroring ? Cart->Emu->SeventyTwoPinConnector[61] : Cart->Emu->SeventyTwoPinConnector[62]; }
        }
        void Mapper::Connector_IRQPin(bool t)
        {
            if (!Cart->Emu->ConnectorPinFloating[14]) { Cart->Emu->SeventyTwoPinConnector[14] = t; Cart->Emu->IRQ_LevelDetector = t; }
        }
        void Mapper::Connector_PPU_RW(bool RD, bool WR)
        {
            if (!Cart->Emu->ConnectorPinFloating[20]) { Cart->Emu->SeventyTwoPinConnector[20] = !RD; }
            if (!Cart->Emu->ConnectorPinFloating[55]) { Cart->Emu->SeventyTwoPinConnector[55] = !WR; }
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