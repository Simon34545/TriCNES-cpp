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

static void disk();