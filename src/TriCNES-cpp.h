#pragma once

#include <iostream>
#include <chrono>

#define SDL_MAIN_USE_CALLBACKS 1 
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "gui.h"

#include "./TriCNES/Emulator.h"
#include "./TriCNES/Math.h"
#include "./TriCNES/mappers.h"

void texcpy(void* dest, void* src, int destPitch, int srcPitch, int height)
{
    for (int i = 0; i < height; i++)
    {
        memcpy(&(((char*)dest)[i * destPitch]), &(((char*)src)[i * srcPitch]), srcPitch);
    }
}

// SDL vars
static SDL_Renderer* renderer;

static SDL_Texture* buffer0;
static SDL_Texture* buffer1;
static SDL_Texture* buffer2;
static SDL_Texture* buffer3;

static SDL_Texture* buffer;

static int* pixels0 = NULL;
static int* pixels1 = NULL;
static int* pixels2 = NULL;
static int* pixels3 = NULL;
static int pitch0;
static int pitch1;
static int pitch2;
static int pitch3;

static SDL_AudioStream* stream;
static SDL_Window* window;

static bool mod_shift = false;
static bool mod_ctrl = false;

#ifdef QTGUI
QApplication* qtApp = NULL;
#endif


// Emulator vars
static TriCNES::Emulator emulator;
static TriCNES::Cartridge cartridge;
static bool powered = false;
static bool paused = false;
static bool vsync = true;
static bool ksync = true;
static std::string BIOS = "";
static bool PendingDiskSelect = false;

static std::vector<byte> states[8];

struct KeyEvent
{
    bool type;
    SDL_Keycode key;
};

std::vector<KeyEvent> deferred_keys;

static MENU_ID tas_caller = NULL;

static ushort* tasInputs;
static bool* tasResets;
static int tasLength;

// Audio vars
static Sint16* bufAudio = new Sint16[512];
static int bufReadIdx = 0;
static int bufWriteIdx = 0;
static int bufHas = 0;
static int bufNeeds = 512;
static double clockt = 1.0 / (21477272.0 / 12.0);
static double samplet = 1.0 / 44100.0;
static double t = 0;

static double speed = 1.0;
static double speeds[16] = { 0.01, 0.03, 0.06, 0.12, 0.25, 0.50, 0.75, 1.00, 1.50, 2.00, 3.00, 4.00, 8.00, 16.00, 32.00, 64.00 };

// Performance vars
static auto t0 = std::chrono::steady_clock::now();
static double avg = 60.0f;
static int total = 0;

static void render();
void sync_keys();

#include "ids.h"

static MENU console;

static MENU tas;
static DropdownSelect tasCPUSelect;
static DropdownSelect tasPPUSelect;
static DropdownSelect tasFilterSelect;
static DropdownSelect tasRAMSelect;

static MENU settings;
static DropdownSelect settingsSpeedSelect;
static DropdownSelect settingsPPUSelect;
static DropdownSelect settingsModeSelect;
static DropdownSelect settingsScaleSelect;

static MENU tools;

MENU_CALLBACK(syncSettings);
MENU_CALLBACK(disk);
MENU_CALLBACK(dummy_handler) {};

#include "tas.h"
#include "menu.h"