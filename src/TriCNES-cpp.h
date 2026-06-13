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

#include "tas.h"

MENU_CALLBACK(disk);

void sync_keys();

MENU_CALLBACK(dummy_handler) {};