#pragma once

#include "ntviewer.h"

MENU_CALLBACK(power)
{

    SDL_LockAudioStream(stream);

    if (!powered)
    {
        emulator.~Emulator();
        new (&emulator) TriCNES::Emulator();

        emulator.Cart = &cartridge;
        cartridge.Emu = &emulator;
        cartridge.MapperChip->Cart = &cartridge;
        if (cartridge.FDS != nullptr) cartridge.FDS->Cart = &cartridge;
        //if (cartridge.FDS != nullptr) cartridge.FDS->CurrentState = TriCNES::DiskDrive::RamAdapterState::RESET;

        emulator.PPUClock = GetDropdownSelectedIndex(settingsPPUSelect);
        syncSettings(BTN_POWER);
    }
    else
    {
        for (int i = 0; i < 256 * 1 * 240; i++) emulator.Screen[i] = 0xFF000000;
        for (int i = 0; i < 341 * 1 * 262; i++) emulator.BorderedScreen[i] = 0xFF000000;
        for (int i = 0; i < 256 * 8 * 240; i++) emulator.NTSCScreen[i] = 0xFF000000;
        for (int i = 0; i < 341 * 8 * 262; i++) emulator.BorderedNTSCScreen[i] = 0xFF000000;
        render();
    }

    powered = !powered;
    SDL_UnlockAudioStream(stream);
}

MENU_CALLBACK(reset)
{
    SDL_LockAudioStream(stream);
    if (powered) emulator.Reset();
    SDL_UnlockAudioStream(stream);
}

static void SDLCALL cartCallback(void* userdata, const char* const* file, int filter)
{
    if (file == NULL || file[0] == NULL) return;

    SDL_LockAudioStream(stream);

    cartridge = TriCNES::Cartridge(file[0]);
    cartridge.Emu = &emulator;
    cartridge.MapperChip->Cart = &cartridge;
    emulator.Cart = &cartridge;

    if (!powered)
    {
        power(NULL);
    }

    SDL_UnlockAudioStream(stream);
}

MENU_CALLBACK(cart)
{
    const SDL_DialogFileFilter filters[] = { {"NES ROMs", "nes;bin"} };
    SDL_ShowOpenFileDialog(cartCallback, NULL, window, filters, 1, NULL, false);
}

MENU_CALLBACK(rmcart)
{
    cartridge = TriCNES::Cartridge();
    cartridge.MapperChip = new TriCNES::Mapper_NULL();

    emulator.Cart = &cartridge;
    cartridge.Emu = &emulator;
    cartridge.MapperChip->Cart = &cartridge;
}

static void SDLCALL biosCallback(void* userdata, const char* const* file, int filter)
{
    if (file == NULL || file[0] == NULL)
    {
        PendingDiskSelect = false;
        return;
    }

    BIOS = file[0];


    if (PendingDiskSelect)
    {
        PendingDiskSelect = false;

        Alert(window, "Success", "BIOS path has been set.", disk);
    }
    else
    {
        Alert(window, "Success", "BIOS path has been set.", dummy_handler);
    }
}

MENU_CALLBACK(bios)
{
    const SDL_DialogFileFilter filters[] = { {"FDS BIOS ROMs", "rom;bin"} };
    SDL_ShowOpenFileDialog(biosCallback, NULL, window, filters, 1, NULL, false);
}

static void SDLCALL diskCallback(void* userdata, const char* const* file, int filter)
{
    if (file == NULL || file[0] == NULL) return;

    SDL_LockAudioStream(stream);

    if (powered && cartridge.FDS != nullptr)
    {
        emulator.Cart = &cartridge;
        cartridge.Emu = &emulator;
        cartridge.MapperChip->Cart = &cartridge;
        cartridge.FDS->Cart = &cartridge;

        cartridge.FDS->InsertDisk(file[0]);
    }
    else
    {
        cartridge = TriCNES::Cartridge(file[0], BIOS);

        if (!powered)
        {
            power(NULL);
        }
        else
        {
            cartridge.Emu = &emulator;
            cartridge.MapperChip->Cart = &cartridge;
            cartridge.FDS->Cart = &cartridge;
        }
    }

    SDL_UnlockAudioStream(stream);
}

MENU_CALLBACK(disk)
{
    if (BIOS.empty())
    {
        PendingDiskSelect = true;
        Alert(window, "Missing FDS BIOS", "Please select an FDS BIOS ROM.", bios);
        return;
    }

    const SDL_DialogFileFilter filters[] = { {"FDS Disk ROMs", "fds;bin"} };
    SDL_ShowOpenFileDialog(diskCallback, NULL, window, filters, 1, NULL, false);
}

MENU_CALLBACK(save)
{
    int i = 0;

    switch (caller)
    {
    case BTN_SAVE0: i = 0; break;
    case BTN_SAVE1: i = 1; break;
    case BTN_SAVE2: i = 2; break;
    case BTN_SAVE3: i = 3; break;
    case BTN_SAVE4: i = 4; break;
    case BTN_SAVE5: i = 5; break;
    case BTN_SAVE6: i = 6; break;
    case BTN_SAVE7: i = 7; break;
    }

    states[i] = emulator.SaveState();
}

MENU_CALLBACK(load)
{
    int i = 0;

    switch (caller)
    {
    case BTN_LOAD0: i = 0; break;
    case BTN_LOAD1: i = 1; break;
    case BTN_LOAD2: i = 2; break;
    case BTN_LOAD3: i = 3; break;
    case BTN_LOAD4: i = 4; break;
    case BTN_LOAD5: i = 5; break;
    case BTN_LOAD6: i = 6; break;
    case BTN_LOAD7: i = 7; break;
    }

    if (states[i].size() != 0) emulator.LoadState(states[i]);
}

MENU_CALLBACK(pause)
{
    paused = !paused;

    SetMenuItemChecked(console, BTN_PAUSE, paused);
}


MENU_CALLBACK(frame0)
{
    SetMenuItemChecked(tas, BTN_FRAME0, !GetMenuItemChecked(tas, BTN_FRAME0));
}

static void SDLCALL tasCallback(void* userdata, const char* const* file, int filter)
{
    if (file == NULL || file[0] == NULL) return;

    SetDropdownSelected(tasFilterSelect, BTN_TASLATCH);
    SetDropdownSelected(tasCPUSelect, BTN_TASCPU0);
    SetDropdownSelected(tasPPUSelect, BTN_TASPPU0);
    SetDropdownSelected(tasRAMSelect, BTN_TASRAM0);

    SetMenuItemChecked(tas, BTN_FRAME0, false);

    tasLength = 0;

    switch (tas_caller)
    {
    case BTN_TAS3CT: tasLength = load3CT(file[0]); break;
    case BTN_TAS3C2: tasLength = load3C2(file[0], tasInputs, tasResets, false); break;
    case BTN_TAS3C3: tasLength = load3C2(file[0], tasInputs, tasResets, true); break;
    case BTN_TASR08: tasLength = loadR08(file[0], tasInputs, tasResets); break;
    case BTN_TASFMV: tasLength = loadFMV(file[0], tasInputs, tasResets); break;
    case BTN_TASFM2: tasLength = loadFM2(file[0], tasInputs, tasResets); break;
    case BTN_TASBK2: tasLength = loadBK2(file[0], tasInputs, tasResets); break;
    default: return;
    }

    if (tasLength == 0) return Alert(window, "Error", "Something went wrong while loading the TAS.", dummy_handler);

    char message[200];
    snprintf(message, sizeof(message), "Loaded %i TAS inputs.\nPlease confirm settings and then press Start TAS.", tasLength);

    Alert(window, "Success", message, dummy_handler);
}

MENU_CALLBACK(loadTAS)
{
    SDL_DialogFileFilter filter;

    switch (caller)
    {
    case BTN_TAS3CT: filter = { "TriCNES Cartswap TAS files", "3ct" }; break;
    case BTN_TAS3C2: filter = { "TriCNES TAS files", "3c2" }; break;
    case BTN_TAS3C3: filter = { "TriCNES TAS Timeline files", "3c3" }; break;
    case BTN_TASR08: filter = { "Replay Device TAS files", "r08" }; break;
    case BTN_TASFMV: filter = { "Famtasia TAS files", "fmv" }; break;
    case BTN_TASFM2: filter = { "FCEUX TAS files", "fm2;fm3" }; break;
    case BTN_TASBK2: filter = { "Bizhawk TAS files", "bk2;tasproj" }; break;
    default: return;
    }

    const SDL_DialogFileFilter filters[] = { filter };

    tas_caller = caller;

    SDL_ShowOpenFileDialog(tasCallback, NULL, window, filters, 1, NULL, false);
}

MENU_CALLBACK(startTAS)
{
    if (tasLength == 0) return Alert(window, "Error", "Please load a TAS first!", loadTAS);

    SDL_LockAudioStream(stream);

    if (caller == BTN_TASSTARTR)
    {
        if (!powered) return Alert(window, "Error", "Please power on the emulator first.", dummy_handler);

        reset(NULL);
    }
    else
    {

        powered = false;
        power(NULL);
    }

    if (tas_caller == BTN_TAS3CT)
    {
        TriCTASCycles = 0;
        TriCTASIndex = 0;
        TriCTASRunning = true;

        for (int i = 0; i < TriCTASCarts.size(); i++)
        {
            TriCTASCarts[i].Emu = &emulator;
            TriCTASCarts[i].MapperChip->Cart = &(TriCTASCarts[i]);
        }
    }
    else
    {
        emulator.TAS_ReadingTAS = true;

        emulator.TAS_InputLogLength = tasLength;
        emulator.TAS_ResetLogLength = tasLength;
        emulator.TAS_InputLog = tasInputs;
        emulator.TAS_ResetLog = tasResets;
    }

    emulator.ClockFiltering = GetDropdownSelected(tasFilterSelect) == BTN_TASCLOCK;

    emulator.PPUClock = GetDropdownSelectedIndex(tasPPUSelect);
    emulator.CPUClock = GetDropdownSelectedIndex(tasCPUSelect);

    emulator.TAS_InputSequenceIndex = 0;

    switch (GetDropdownSelected(tasRAMSelect))
    {
    case BTN_TASRAM0:
        // Default TriCNES RAM pattern
        // (handled by emulator constructor)
        break;
    case BTN_TASRAM1:
        // Bizhawk and FCEUX RAM pattern
        for (int i = 0; i < 0x800; i++) emulator.RAM[i] = ((i & 7) > 4) ? 0xFF : 0x00;
        break;
    case BTN_TASRAM2:
        // Initial RAM pattern for the Super Mario Bros. Bad Apple TAS
        memcpy(emulator.RAM, SMB1RAM, 0x800);
        break;
    }

    if (GetMenuItemChecked(tas, BTN_FRAME0))
    {
        emulator.PPU_Scanline = 239;
        emulator.PPU_Dot = 312;

        emulator.SyncFM2 = true;
        emulator.TAS_InputSequenceIndex--;
    }
    else if (tas_caller == BTN_TASFM2)
    {
        emulator.TAS_InputSequenceIndex++;
        emulator.PPU_Dot = 0;
    }

    SDL_UnlockAudioStream(stream);
}

MENU_CALLBACK(syncSettings)
{
    SDL_LockAudioStream(stream);

    emulator.PPU_ShowRawNTSCSignal = GetMenuItemChecked(settings, BTN_NTSCRAW);

    speed = speeds[GetDropdownSelectedIndex(settingsSpeedSelect)];

    int mode = GetDropdownSelectedIndex(settingsModeSelect);

    emulator.PPU_DecodeSignal = mode > 1;
    emulator.PPU_ShowScreenBorders = mode == 1 || mode == 3;

    int width = emulator.PPU_ShowScreenBorders ? 341 : 256;
    int height = emulator.PPU_ShowScreenBorders ? 262 : 240;

    int scale = GetDropdownSelectedIndex(settingsScaleSelect);

    if (scale == 0)
    {
        width *= emulator.PPU_DecodeSignal ? 1 : 8;
        height *= 7;
    }
    else
    {
        width *= scale;
        height *= scale;
    }

    if (caller != BTN_POWER) SDL_SetWindowSize(window, width, height);

    switch (mode)
    {
    case 0: buffer = buffer0; break;
    case 1: buffer = buffer1; break;
    case 2: buffer = buffer2; break;
    case 3: buffer = buffer3; break;
    }

    SDL_UnlockAudioStream(stream);
}

MENU_CALLBACK(setVSync)
{
    vsync = !vsync;
    SetMenuItemChecked(settings, BTN_FRAME0, vsync);
}

MENU_CALLBACK(setKSync)
{
    ksync = !ksync;
    SetMenuItemChecked(settings, BTN_FRAME0, ksync);
}

MENU_CALLBACK(setRaw)
{
    SetMenuItemChecked(settings, BTN_NTSCRAW, !GetMenuItemChecked(settings, BTN_NTSCRAW));
    syncSettings(NULL);
}


MENU_CALLBACK(setPPU)
{
    powered = false;
    power(NULL);
}

void InitMenuBar()
{

    MENU menuBar = CreateMenuBar(window);

    console = AddMenu(menuBar, "Console");

    AddMenuItem(console, BTN_POWER, "Power\tCtrl+P", power);
    AddMenuItem(console, BTN_RESET, "Reset\tCtrl+R", reset);
    AddMenuItem(console, BTN_CART, "Insert Cartridge", cart);
    AddMenuItem(console, BTN_RMCART, "Remove Cartridge", rmcart);
    AddMenuItem(console, BTN_BIOS, "Load FDS BIOS", bios);
    AddMenuItem(console, BTN_DISK, "Insert FDS Disk", disk);
    AddMenuItem(console, BTN_PAUSE, "Pause\tP", pause); SetMenuItemChecked(console, BTN_PAUSE, paused);

    MENU saveStates = AddSubMenu(console, "Save State");
    AddMenuItem(saveStates, BTN_SAVE0, "Slot 0\tCtrl+Shift+1", save);
    AddMenuItem(saveStates, BTN_SAVE1, "Slot 1\tCtrl+Shift+2", save);
    AddMenuItem(saveStates, BTN_SAVE2, "Slot 2\tCtrl+Shift+3", save);
    AddMenuItem(saveStates, BTN_SAVE3, "Slot 3\tCtrl+Shift+4", save);
    AddMenuItem(saveStates, BTN_SAVE4, "Slot 4\tCtrl+Shift+5", save);
    AddMenuItem(saveStates, BTN_SAVE5, "Slot 5\tCtrl+Shift+6", save);
    AddMenuItem(saveStates, BTN_SAVE6, "Slot 6\tCtrl+Shift+7", save);
    AddMenuItem(saveStates, BTN_SAVE7, "Slot 7\tCtrl+Shift+8", save);

    MENU loadStates = AddSubMenu(console, "Load State");
    AddMenuItem(loadStates, BTN_LOAD0, "Slot 0\tCtrl+1", load);
    AddMenuItem(loadStates, BTN_LOAD1, "Slot 1\tCtrl+2", load);
    AddMenuItem(loadStates, BTN_LOAD2, "Slot 2\tCtrl+3", load);
    AddMenuItem(loadStates, BTN_LOAD3, "Slot 3\tCtrl+4", load);
    AddMenuItem(loadStates, BTN_LOAD4, "Slot 4\tCtrl+5", load);
    AddMenuItem(loadStates, BTN_LOAD5, "Slot 5\tCtrl+6", load);
    AddMenuItem(loadStates, BTN_LOAD6, "Slot 6\tCtrl+7", load);
    AddMenuItem(loadStates, BTN_LOAD7, "Slot 7\tCtrl+8", load);

    tas = AddMenu(menuBar, "TAS");

    MENU tasLoad = AddSubMenu(tas, "Load TAS");
    AddMenuItem(tasLoad, BTN_TAS3CT, "TriCNES Intercycle Cartridge Swapping TAS\t*.3ct", loadTAS);
    AddMenuItem(tasLoad, BTN_TAS3C2, "TriCNES TAS (beta)\t*.3c2", loadTAS);
    AddMenuItem(tasLoad, BTN_TAS3C3, "TriCNES TAS Timeline (beta)\t*.3c3", loadTAS);
    AddMenuItem(tasLoad, BTN_TASR08, "Replay Device\t*.r08", loadTAS);
    AddMenuItem(tasLoad, BTN_TASFMV, "Famtasia\t*.fmv", loadTAS);
    AddMenuItem(tasLoad, BTN_TASFM2, "FCEUX\t*.fm2;*.fm3", loadTAS);
    AddMenuItem(tasLoad, BTN_TASBK2, "Bizhawk\t*.bk2;*.tasproj", loadTAS);

    AddMenuItem(tas, BTN_FRAME0, "FCEUX Frame 0 Timing", frame0);

    MENU tasCPU = AddSubMenu(tas, "CPU Clock Alignment");
    AddMenuItem(tasCPU, BTN_TASCPU0, "Phase 0");
    AddMenuItem(tasCPU, BTN_TASCPU1, "Phase 1");
    AddMenuItem(tasCPU, BTN_TASCPU2, "Phase 2");
    AddMenuItem(tasCPU, BTN_TASCPU3, "Phase 3");
    AddMenuItem(tasCPU, BTN_TASCPU4, "Phase 4");
    AddMenuItem(tasCPU, BTN_TASCPU5, "Phase 5");
    AddMenuItem(tasCPU, BTN_TASCPU6, "Phase 6");
    AddMenuItem(tasCPU, BTN_TASCPU7, "Phase 7");
    AddMenuItem(tasCPU, BTN_TASCPU8, "Phase 8");
    AddMenuItem(tasCPU, BTN_TASCPU9, "Phase 9");
    AddMenuItem(tasCPU, BTN_TASCPU10, "Phase 10");
    AddMenuItem(tasCPU, BTN_TASCPU11, "Phase 11");

    MENU_ID* cpuMenus = new MENU_ID[]{ BTN_TASCPU0, BTN_TASCPU1, BTN_TASCPU2, BTN_TASCPU3, BTN_TASCPU4, BTN_TASCPU5,
                                       BTN_TASCPU6, BTN_TASCPU7, BTN_TASCPU8, BTN_TASCPU9, BTN_TASCPU10, BTN_TASCPU11 };

    tasCPUSelect = {
        tasCPU,
        cpuMenus,
        12,
        dummy_handler
    };

    CreateDropdownSelectHandler(tasCPUSelect);

    MENU tasPPU = AddSubMenu(tas, "PPU Clock Alignment");
    AddMenuItem(tasPPU, BTN_TASPPU0, "Phase 0");
    AddMenuItem(tasPPU, BTN_TASPPU1, "Phase 1");
    AddMenuItem(tasPPU, BTN_TASPPU2, "Phase 2");
    AddMenuItem(tasPPU, BTN_TASPPU3, "Phase 3");

    MENU_ID* ppuMenus = new MENU_ID[]{ BTN_TASPPU0, BTN_TASPPU1, BTN_TASPPU2, BTN_TASPPU3 };

    tasPPUSelect = {
        tasPPU,
        ppuMenus,
        4,
        dummy_handler
    };

    CreateDropdownSelectHandler(tasPPUSelect);


    MENU tasFilter = AddSubMenu(tas, "Filter Mode");
    AddMenuItem(tasFilter, BTN_TASLATCH, "Latch Filtering");
    AddMenuItem(tasFilter, BTN_TASCLOCK, "Clock Filtering");

    MENU_ID* filterMenus = new MENU_ID[]{ BTN_TASLATCH, BTN_TASCLOCK };

    tasFilterSelect = {
        tasFilter,
        filterMenus,
        2,
        dummy_handler
    };

    CreateDropdownSelectHandler(tasFilterSelect);


    MENU tasRAM = AddSubMenu(tas, "Initial RAM Pattern");
    AddMenuItem(tasRAM, BTN_TASRAM0, "TriCNES");
    AddMenuItem(tasRAM, BTN_TASRAM1, "Bizhawk / FCEUX");
    AddMenuItem(tasRAM, BTN_TASRAM2, "SMB1 ACE Setup");

    MENU_ID* ramMenus = new MENU_ID[]{ BTN_TASRAM0, BTN_TASRAM1, BTN_TASRAM2 };

    tasRAMSelect = {
        tasRAM,
        ramMenus,
        3,
        dummy_handler
    };

    CreateDropdownSelectHandler(tasRAMSelect);


    MENU tasStart = AddSubMenu(tas, "Start TAS");
    AddMenuItem(tasStart, BTN_TASSTARTP, "From POWER", startTAS);
    AddMenuItem(tasStart, BTN_TASSTARTR, "From RESET", startTAS);

    settings = AddMenu(menuBar, "Settings");

    AddMenuItem(settings, BTN_VSYNC, "Picture VSync", setVSync);
    AddMenuItem(settings, BTN_KSYNC, "Input VSync", setKSync);
    AddMenuItem(settings, BTN_NTSCRAW, "Show Raw NTSC Signal", setRaw);

    MENU settingsSpeed = AddSubMenu(settings, "Emulation Speed");
    AddMenuItem(settingsSpeed, BTN_SPEED0, "1%");
    AddMenuItem(settingsSpeed, BTN_SPEED1, "3%");
    AddMenuItem(settingsSpeed, BTN_SPEED2, "6%");
    AddMenuItem(settingsSpeed, BTN_SPEED3, "12%");
    AddMenuItem(settingsSpeed, BTN_SPEED4, "25%");
    AddMenuItem(settingsSpeed, BTN_SPEED5, "50%");
    AddMenuItem(settingsSpeed, BTN_SPEED6, "75%");
    AddMenuItem(settingsSpeed, BTN_SPEED7, "100%");
    AddMenuItem(settingsSpeed, BTN_SPEED8, "150%");
    AddMenuItem(settingsSpeed, BTN_SPEED9, "200%");
    AddMenuItem(settingsSpeed, BTN_SPEEDA, "300%");
    AddMenuItem(settingsSpeed, BTN_SPEEDB, "400%");
    AddMenuItem(settingsSpeed, BTN_SPEEDC, "800%");
    AddMenuItem(settingsSpeed, BTN_SPEEDD, "1600%");
    AddMenuItem(settingsSpeed, BTN_SPEEDE, "3200%");
    AddMenuItem(settingsSpeed, BTN_SPEEDF, "6400%");

    MENU_ID* speedMenus = new MENU_ID[]{ BTN_SPEED0, BTN_SPEED1, BTN_SPEED2, BTN_SPEED3,
                                         BTN_SPEED4, BTN_SPEED5, BTN_SPEED6, BTN_SPEED7,
                                         BTN_SPEED8, BTN_SPEED9, BTN_SPEEDA, BTN_SPEEDB,
                                         BTN_SPEEDC, BTN_SPEEDD, BTN_SPEEDE, BTN_SPEEDF };

    settingsSpeedSelect = {
        settingsSpeed,
        speedMenus,
        16,
        syncSettings
    };

    CreateDropdownSelectHandler(settingsSpeedSelect);
    SetDropdownSelectedIndex(settingsSpeedSelect, 7);

    MENU settingsPPU = AddSubMenu(settings, "PPU Clock");
    AddMenuItem(settingsPPU, BTN_SPPU0, "Phase 0");
    AddMenuItem(settingsPPU, BTN_SPPU1, "Phase 1");
    AddMenuItem(settingsPPU, BTN_SPPU2, "Phase 2");
    AddMenuItem(settingsPPU, BTN_SPPU3, "Phase 3");

    MENU_ID* sppuMenus = new MENU_ID[]{ BTN_SPPU0, BTN_SPPU1, BTN_SPPU2, BTN_SPPU3 };

    settingsPPUSelect = {
        settingsPPU,
        sppuMenus,
        4,
        setPPU
    };

    CreateDropdownSelectHandler(settingsPPUSelect);
    SetDropdownSelectedIndex(settingsPPUSelect, 0);

    MENU settingsMode = AddSubMenu(settings, "Picture Mode");
    AddMenuItem(settingsMode, BTN_MODE0, "RGB\t256 x 240");
    AddMenuItem(settingsMode, BTN_MODE1, "RGB Uncropped\t341 x 262");
    AddMenuItem(settingsMode, BTN_MODE2, "NTSC\t256 x 8 x 240");
    AddMenuItem(settingsMode, BTN_MODE3, "NTSC Uncropped\t341 x 8 x 262");

    MENU_ID* modeMenus = new MENU_ID[]{ BTN_MODE0, BTN_MODE1, BTN_MODE2, BTN_MODE3 };

    settingsModeSelect = {
        settingsMode,
        modeMenus,
        4,
        syncSettings
    };

    CreateDropdownSelectHandler(settingsModeSelect);
    SetDropdownSelectedIndex(settingsModeSelect, 0);


    MENU settingsScale = AddSubMenu(settings, "Picture Scale");
    AddMenuItem(settingsScale, BTN_SCALE0, "True 8:7 PAR");
    AddMenuItem(settingsScale, BTN_SCALE1, "1x");
    AddMenuItem(settingsScale, BTN_SCALE2, "2x");
    AddMenuItem(settingsScale, BTN_SCALE3, "3x");
    AddMenuItem(settingsScale, BTN_SCALE4, "4x");
    AddMenuItem(settingsScale, BTN_SCALE5, "5x");
    AddMenuItem(settingsScale, BTN_SCALE6, "6x");
    AddMenuItem(settingsScale, BTN_SCALE7, "7x");
    AddMenuItem(settingsScale, BTN_SCALE8, "8x");
    
    MENU_ID* scaleMenus = new MENU_ID[]{ BTN_SCALE0, BTN_SCALE1, BTN_SCALE2, BTN_SCALE3, BTN_SCALE4, BTN_SCALE5, BTN_SCALE6, BTN_SCALE7, BTN_SCALE8 };

    settingsScaleSelect = {
        settingsScale,
        scaleMenus,
        9,
        syncSettings
    };

    CreateDropdownSelectHandler(settingsScaleSelect);
    SetDropdownSelectedIndex(settingsScaleSelect, 1);

    tools = AddMenu(menuBar, "Tools");

    AddMenuItem(tools, BTN_NTVIEWER, "Nametable Viewer", openNTViewer);
}