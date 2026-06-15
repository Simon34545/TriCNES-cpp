#include "TriCNES-cpp.h"

static void frame()
{
    auto t1 = std::chrono::steady_clock::now();

    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    avg += (1000000.0 / deltaTime);
    total++;

    if (total >= 60) {
        avg /= 60.0;

        printf("%.2f FPS\n", avg);

        avg = 0.0f;
        total = 0;
    }

    t0 = t1;
}

static void render()
{
    SDL_LockTexture(buffer,
        NULL,
        (void**)&pixels,
        &pitch);

    memcpy(pixels, emulator.Screen, 256 * 240 * sizeof(int));

    SDL_UnlockTexture(buffer);
    SDL_RenderTexture(renderer, buffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void fillBuffer()
{
    while (bufNeeds > 0)
    {
        for (int i = 0; i < bufNeeds; i++)
        {
            while (t < samplet)
            {
                if (powered && !paused)
                {
                    if (TriCTASRunning)
                    {
                        if (++TriCTASCycles == TriCTASSwaps[TriCTASIndex])
                        {
                            emulator.Cart = &(TriCTASCarts[TriCTASIndices[TriCTASIndex]]);
                            if (++TriCTASIndex == TriCTASSwaps.size()) TriCTASRunning = false;
                        }
                    }

                    emulator._CoreCycleAdvance();

                    if (emulator.FrameAdvance_ReachedVBlank)
                    {
                        if (vsync) render();
                        if (ksync) sync_keys();
                        frame();
                        emulator.FrameAdvance_ReachedVBlank = false;
                    }
                }
                t += clockt;
            }
            t -= samplet;

            double sample = emulator.APU_DMC_Output / 255.0 * 32767.0;

            bufAudio[bufWriteIdx++] = sample;

            if (bufWriteIdx == 512) bufWriteIdx = 0;

            bufHas++;
            bufNeeds--;
        }
    }
}

static void SDLCALL audio(void* userdata, SDL_AudioStream* stream, int len, int total)
{
    if (len > 0 && len / 2 <= bufHas)
    {
        Uint8* data = SDL_stack_alloc(Uint8, len);
        
        if (data)
        {
            for (int i = 0; i < len / 2; i++)
            {
                
                // Write the sample to the audio buffer (16-bit signed)
                ((Sint16*)data)[i] = bufAudio[bufReadIdx++];

                if (bufReadIdx == 512) bufReadIdx = 0;

                bufHas--;
                bufNeeds++;
            }

            SDL_PutAudioStreamData(stream, data, len);
            SDL_stack_free(data);
        }
    }
}

static void keyDown(SDL_Keycode key)
{

    if (mod_ctrl)
    {
        if (mod_shift)
        {
            switch (key)
            {
            case SDLK_1: save(BTN_SAVE0); break;
            case SDLK_2: save(BTN_SAVE1); break;
            case SDLK_3: save(BTN_SAVE2); break;
            case SDLK_4: save(BTN_SAVE3); break;
            case SDLK_5: save(BTN_SAVE4); break;
            case SDLK_6: save(BTN_SAVE5); break;
            case SDLK_7: save(BTN_SAVE6); break;
            case SDLK_8: save(BTN_SAVE7); break;
            }
        }
        else
        {
            switch (key)
            {
            case SDLK_R: reset(NULL); break;
            case SDLK_P: power(NULL); break;
            case SDLK_1: load(BTN_LOAD0); break;
            case SDLK_2: load(BTN_LOAD1); break;
            case SDLK_3: load(BTN_LOAD2); break;
            case SDLK_4: load(BTN_LOAD3); break;
            case SDLK_5: load(BTN_LOAD4); break;
            case SDLK_6: load(BTN_LOAD5); break;
            case SDLK_7: load(BTN_LOAD6); break;
            case SDLK_8: load(BTN_LOAD7); break;
            }
        }
    }

    switch (key)
    {
    case SDLK_LSHIFT: mod_shift = true; break;
    case SDLK_LCTRL:  mod_ctrl = true; break;
    case SDLK_P: pause(NULL); break;
    }

    if (emulator.TAS_ReadingTAS) return;

    switch (key)
    {
    case SDLK_X:      emulator.ControllerPort1 |= 0x80; break;
    case SDLK_Z:      emulator.ControllerPort1 |= 0x40; break;
    case SDLK_RSHIFT: emulator.ControllerPort1 |= 0x20; break;
    case SDLK_RETURN: emulator.ControllerPort1 |= 0x10; break;
    case SDLK_UP:     emulator.ControllerPort1 |= 0x08; break;
    case SDLK_DOWN:   emulator.ControllerPort1 |= 0x04; break;
    case SDLK_LEFT:   emulator.ControllerPort1 |= 0x02; break;
    case SDLK_RIGHT:  emulator.ControllerPort1 |= 0x01; break;
    }
}

static void keyUp(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_LSHIFT: mod_shift = false; break;
    case SDLK_LCTRL:  mod_ctrl = false; break;
    }

    if (emulator.TAS_ReadingTAS) return;

    switch (key)
    {
    case SDLK_X:      emulator.ControllerPort1 &= ~0x80; break;
    case SDLK_Z:      emulator.ControllerPort1 &= ~0x40; break;
    case SDLK_RSHIFT: emulator.ControllerPort1 &= ~0x20; break;
    case SDLK_RETURN: emulator.ControllerPort1 &= ~0x10; break;
    case SDLK_UP:     emulator.ControllerPort1 &= ~0x08; break;
    case SDLK_DOWN:   emulator.ControllerPort1 &= ~0x04; break;
    case SDLK_LEFT:   emulator.ControllerPort1 &= ~0x02; break;
    case SDLK_RIGHT:  emulator.ControllerPort1 &= ~0x01; break;
    }
}

void sync_keys()
{
    while (deferred_keys.size() > 0)
    {
        KeyEvent e = deferred_keys[0];
        deferred_keys.erase(deferred_keys.begin());

        (e.type ? keyDown : keyUp)(e.key);
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    cartridge.MapperChip = new TriCNES::Mapper_NULL();

    if (argc > 1)
    {
        if (argv[1][0] == 'n')
        {
            if (argc < 3)
            {
                std::cout << "USAGE: TriCNES-cpp.exe nes rom.nes" << std::endl;
                return SDL_APP_SUCCESS;
            }

            cartridge = TriCNES::Cartridge(argv[2]);

            emulator.Cart = &cartridge;
            cartridge.Emu = &emulator;
            cartridge.MapperChip->Cart = &cartridge;
            powered = true;
        }
        else if (argv[1][0] == 'f')
        {
            if (argc < 4)
            {
                std::cout << "USAGE: TriCNES-cpp.exe fds rom.fds bios.rom" << std::endl;
                return SDL_APP_SUCCESS;
            }

            cartridge = TriCNES::Cartridge(argv[2], argv[3]);

            emulator.Cart = &cartridge;
            cartridge.Emu = &emulator;
            cartridge.MapperChip->Cart = &cartridge;
            powered = true;
        }
        else
        {
            std::cout << "USAGE: TriCNES-cpp.exe nes|fds <...args>" << std::endl;

            return SDL_APP_SUCCESS;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cout << "SDL could not be initialized!" << std::endl << "SDL_Error: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    window = SDL_CreateWindow("TriCNES C++", 256, 240, NULL);

    if (window == NULL)
    {
        std::cout << "Window could not be created!" << std::endl << "SDL_Error: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, NULL);

    if (renderer == NULL)
    {
        std::cout << "Renderer could not be created!" << std::endl << "SDL_Error: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec req;

    SDL_zero(req);

    req.freq = 44100;
    req.format = SDL_AUDIO_S16LE;
    req.channels = 1;

    const SDL_AudioSpec spec = { SDL_AUDIO_S16LE, 2, 44100 };
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &req, audio, NULL);

    if (stream == NULL)
    {
        std::cout << "SDL_OpenAudioDeviceStream failed: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }
    
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));

    buffer = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256,
        240);

    InitMenuBar();

    SDL_SetWindowSize(window, 256, 240);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    SDL_LockAudioStream(stream);
    fillBuffer();
    if (!vsync) render();
    SDL_UnlockAudioStream(stream);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* e)
{
    if (e->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    else if (e->type == SDL_EVENT_KEY_DOWN)
    {
        if (!ksync || !powered || paused)
            keyDown(e->key.key);
        else
            deferred_keys.push_back({ true, e->key.key });
    }
    else if (e->type == SDL_EVENT_KEY_UP)
    {
        if (!ksync || !powered || paused)
            keyUp(e->key.key);
        else
            deferred_keys.push_back({ false, e->key.key });
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) { ; };