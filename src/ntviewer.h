#pragma once

#define BTN_NTBACKDROP 4020
#define BTN_NTBOUNDARY 4021
#define BTN_NTSOVERLAY 4022

#define CHECKBOX_CALLBACK(name, menu, button) MENU_CALLBACK(name) {\
	SetMenuItemChecked(menu, button, !GetMenuItemChecked(menu, button));\
}

static SDL_Window* NTWindow = NULL;
static SDL_Renderer* NTRenderer = NULL;
static SDL_Texture* NTTexture = NULL;

static MENU NTSettings;

static unsigned int* NTPixels = NULL;
static int NTPitch;

void RenderNametable()
{
	SDL_LockTexture(NTTexture, NULL, (void**)&NTPixels, &NTPitch);

    int tx = 0;
    int ty = 0;
    int x = 0;
    int y = 0;
    int px = 0;
    int py = 0;

    int PatternTile;
    int pal = 0;

    bool ForceBackdropOnIndex0 = GetMenuItemChecked(NTSettings, BTN_NTBACKDROP);

    while (ty < 2)
    {
        while (tx < 2)
        {
            while (y < 30)
            {
                while (x < 32)
                {
                    PatternTile = emulator.ObservePPU((ushort)(0x2000 + 0x400 * tx + 0x800 * ty + x + y * 32));
                    pal = emulator.ObservePPU((ushort)(0x2000 + 0x400 * (tx + 1) + 0x800 * ty - 0x40 + x / 4 + (y / 4) * 8));
                    if ((x & 3) >= 2)
                    {
                        pal = pal >> 2;
                    }
                    if ((y & 3) >= 2)
                    {
                        pal = pal >> 4;
                    }
                    pal = pal & 3;
                    while (py < 8)
                    {
                        while (px < 8)
                        {

                            int k = ((emulator.ObservePPU((ushort)(py + PatternTile * 16 + (!emulator.PPU_PatternSelect_Background ? 0 : 0x1000))) >> (7 - px)) & 1) + 2 * ((emulator.ObservePPU((ushort)(py + 8 + PatternTile * 16 + (!emulator.PPU_PatternSelect_Background ? 0 : 0x1000))) >> (7 - px)) & 1);
                            if (k == 0 && ForceBackdropOnIndex0)
                            {
                                k = emulator.ObservePPU(0x3F00);
                            }
                            else
                            {
                                k = emulator.ObservePPU((ushort)(0x3F00 + k + pal * 4));
                            }
                            unsigned int col = TriCNES::Emulator::NesPalInts[k & 0x3F];

                            SetPixel(NTPixels, NTPitch / sizeof(int), tx * 0x100 + x * 8 + px, ty * 0xF0 + y * 8 + py, col);
                            px++;
                        }
                        px = 0;
                        py++;
                    }
                    py = 0;
                    x++;
                }

                x = 0;
                y++;
            }
            y = 0;
            tx++;
        }
        tx = 0;
        ty++;
    }


    bool DrawScreenBoundary = GetMenuItemChecked(NTSettings, BTN_NTBOUNDARY);
    if (DrawScreenBoundary)
    {
        // convert the t register into X,Y coordinates
        /*
        The v and t registers are 15 bits:
        yyy NN YYYYY XXXXX
        ||| || ||||| +++++-- coarse X scroll
        ||| || +++++-------- coarse Y scroll
        ||| ++-------------- nametable select
        +++----------------- fine Y scroll
        */

        int X = ((emulator.PPU_t & 0b11111) << 3) | emulator.PPU_FineXScroll | ((emulator.PPU_t & 0b10000000000) >> 2);
        int Y = ((emulator.PPU_t & 0b1111100000) >> 2) | ((emulator.PPU_t & 0b111000000000000) >> 12) | ((emulator.PPU_t & 0b100000000000) >> 4);
        
        int i = 0;
        while (i <= 257)
        {
            SetPixel(NTPixels, NTPitch / sizeof(int), (X + 511 + i) & 511, (Y + 479) % 480, 0xFFFFFFFF);
            SetPixel(NTPixels, NTPitch / sizeof(int), (X + 511 + i) & 511, (Y + 240) % 480, 0xFFFFFFFF);
            i++;
        }
        i = 0;
        while (i <= 241)
        {
            SetPixel(NTPixels, NTPitch / sizeof(int), (X + 511) & 511, (Y + 479 + i) % 480, 0xFFFFFFFF);
            SetPixel(NTPixels, NTPitch / sizeof(int), (X + 256) & 511, (Y + 479 + i) % 480, 0xFFFFFFFF);
            i++;
        }
    }

    if (GetMenuItemChecked(NTSettings, BTN_NTSOVERLAY))
    {
        int X = ((emulator.PPU_t & 0b11111) << 3) | emulator.PPU_FineXScroll | ((emulator.PPU_t & 0b10000000000) >> 2);
        int Y = ((emulator.PPU_t & 0b1111100000) >> 2) | ((emulator.PPU_t & 0b111000000000000) >> 12) | ((emulator.PPU_t & 0b100000000000) >> 4);

        for (int xx = 0; xx < 256; xx++)
        {
            for (int yy = 0; yy < 240; yy++)
            {
                SetPixel(NTPixels, NTPitch / sizeof(int), (X + xx) & 511, (Y + yy) % 480, emulator.Screen[yy * 256 + xx]);
            }
        }
    }

	SDL_UnlockTexture(NTTexture);

	SDL_RenderTexture(NTRenderer, NTTexture, NULL, NULL);
	SDL_RenderPresent(NTRenderer);
}

CHECKBOX_CALLBACK(setNTBackdrop, NTSettings, BTN_NTBACKDROP);
CHECKBOX_CALLBACK(setNTBoundary, NTSettings, BTN_NTBOUNDARY);
CHECKBOX_CALLBACK(setNTSOverlay, NTSettings, BTN_NTSOVERLAY);

MENU_CALLBACK(openNTViewer)
{
	if (NTWindow != NULL) return;

	NTWindow = SDL_CreateWindow("Nametable Viewer", 512, 480, NULL);
	if (!NTWindow) return Alert(window, "Couldn't initialize window.", SDL_GetError(), dummy_handler);

	NTRenderer = SDL_CreateRenderer(NTWindow, NULL);
	if (!NTRenderer) return Alert(window, "Couldn't initialize renderer.", SDL_GetError(), dummy_handler);

	NTTexture = SDL_CreateTexture(NTRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 512, 480);

	MENU menuBar = CreateMenuBar(NTWindow);

	NTSettings = AddMenu(menuBar, "Settings");
	AddMenuItem(NTSettings, BTN_NTBACKDROP, "Use Backdrop Color", setNTBackdrop);
	AddMenuItem(NTSettings, BTN_NTBOUNDARY, "Draw Screen Boundary", setNTBoundary);
	AddMenuItem(NTSettings, BTN_NTSOVERLAY, "Overlay Screen", setNTSOverlay);

	SDL_SetWindowSize(NTWindow, 512, 480); // This refreshes the window, SDL will factor in the offset created by the menu bar
}

void closeNTViewer()
{
	if (NTWindow == NULL) return;

	SDL_DestroyTexture(NTTexture);
	SDL_DestroyRenderer(NTRenderer);
	SDL_DestroyWindow(NTWindow);
	NTWindow = NULL;
}