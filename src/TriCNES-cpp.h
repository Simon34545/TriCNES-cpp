#pragma once

#include <iostream>
#include <chrono>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "gui.h"

#include "./TriCNES/Emulator.h"
#include "./TriCNES/Math.h"
#include "./TriCNES/mappers.h"

static void disk();