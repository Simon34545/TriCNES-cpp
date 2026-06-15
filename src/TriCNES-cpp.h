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

// SDL vars
static SDL_Renderer* renderer;
static SDL_Texture* buffer;
static int* pixels = NULL;
static int pitch = 256 * sizeof(int);
static SDL_AudioStream* stream;
static SDL_Window* window;

static bool mod_shift = false;
static bool mod_ctrl = false;

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

// Performance vars
static auto t0 = std::chrono::steady_clock::now();
static double avg = 60.0f;
static int total = 0;

static void render();
void sync_keys();

#include "ids.h"

static HMENU tas;
static DropdownSelect tasCPUSelect;
static DropdownSelect tasPPUSelect;
static DropdownSelect tasFilterSelect;
static DropdownSelect tasRAMSelect;

MENU_CALLBACK(disk);
MENU_CALLBACK(dummy_handler) {};

#include "tas.h"
#include "menu.h"