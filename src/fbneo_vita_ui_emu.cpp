//
// Created by cpasjuste on 01/06/18.
//

#include "burner.h"
#include "burnint.h"

#include "runtime/runtime.h"
#include "fbneo_vita_ui_emu.h"
#include "fbneo_vita_ui_video.h"
#include "fbneo_vita_utility.h"
#include "retro_input_wrapper.h"
#include "rgui_main.h"

using namespace c2d;
using namespace pemu;

int nVidFullscreen = 0;
INT32 bVidUseHardwareGamma = 1;

UINT32 (__cdecl *VidHighCol)(INT32 r, INT32 g, INT32 b, INT32 i);

INT32 VidRecalcPal() { return BurnRecalcPal(); }

#ifdef __FBNEO_VITA_ARM__
extern int nSekCpuCore;

static bool isHardware(int hardware, int type) {
    return (((hardware | HARDWARE_PREFIX_CARTRIDGE) ^ HARDWARE_PREFIX_CARTRIDGE)
            & 0xff000000) == (unsigned int) type;
}

#endif

static UINT32 myHighCol16(int r, int g, int b, int /* i */) {
    UINT32 t;
    t = (r << 8) & 0xf800;
    t |= (g << 3) & 0x07e0;
    t |= (b >> 3) & 0x001f;
    return t;
}

static UiMain *uiInstance;

FBNeoVitaUiEmu::FBNeoVitaUiEmu(UiMain *ui) : UiEmu(ui) {
    printf("FBNeoVitaUiEmu()\n");
    uiInstance = ui;
}

#ifdef __FBNEO_VITA_ARM__

int FBNeoVitaUiEmu::getSekCpuCore() {
    int sekCpuCore = 0; // SEK_CORE_C68K: USE CYCLONE ARM ASM M68K CORE

    std::vector<std::string> zipList;
    int hardware = BurnDrvGetHardwareCode();

    std::string bios = pMain->getConfig()->get(PEMUConfig::OptId::EMU_NEOBIOS, true)->getString();
    if (isHardware(hardware, HARDWARE_PREFIX_SNK) && Utility::contains(bios, "UNIBIOS")) {
        sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
    }

    if (isHardware(hardware, HARDWARE_PREFIX_SEGA_MEGADRIVE)) {
        sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
    } else if (isHardware(hardware, HARDWARE_PREFIX_SEGA)) {
        if (hardware & HARDWARE_SEGA_FD1089A_ENC
            || hardware & HARDWARE_SEGA_FD1089B_ENC
            || hardware & HARDWARE_SEGA_MC8123_ENC
            || hardware & HARDWARE_SEGA_FD1094_ENC
            || hardware & HARDWARE_SEGA_FD1094_ENC_CPU2) {
            sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
            pMain->getUiMessageBox()->show(
                    "WARNING", "ROM IS CRYPTED, USE DECRYPTED ROM (CLONE)\n"
                               "TO ENABLE CYCLONE ASM CORE (FASTER)", "OK");
        }
    } else if (isHardware(hardware, HARDWARE_PREFIX_TOAPLAN)) {
        zipList.emplace_back("batrider");
        zipList.emplace_back("bbakraid");
        zipList.emplace_back("bgaregga");
    } else if (isHardware(hardware, HARDWARE_PREFIX_SNK)) {
        zipList.emplace_back("kof97");
        zipList.emplace_back("kof98");
        zipList.emplace_back("kof99");
        zipList.emplace_back("kof2000");
        zipList.emplace_back("kof2001");
        zipList.emplace_back("kof2002");
        zipList.emplace_back("kf2k3pcb");
        //zipList.push_back("kof2003"); // WORKS
        zipList.emplace_back("mslug5");
        zipList.emplace_back("svc"); // SvC Chaos also uses PVC
    }

    std::string zip = BurnDrvGetTextA(DRV_NAME);
    for (unsigned int i = 0; i < zipList.size(); i++) {
        if (zipList[i].compare(0, zip.length(), zip) == 0) {
            pMain->getUiStatusBox()->show("THIS GAME DOES NOT SUPPORT THE M68K ASM CORE\n"
                                       "CYCLONE ASM CORE DISABLED");
            sekCpuCore = 1; // SEK_CORE_M68K: USE C M68K CORE
            break;
        }
    }

    zipList.clear();

    return sekCpuCore;
}

#endif

int FBNeoVitaUiEmu::load(const Game &game) {
    fbneo_vita::load_error::clear();
    currentGame = game;

    FBNeoVitaUtility::setDriverActive(game);
    if (nBurnDrvActive >= nBurnDrvCount) {
        printf("FBNeoVitaUiEmu::load: driver not found\n");
        fbneo_vita::load_error::set(
                "Unsupported Game",
                "This ROM is not supported by FBNeo.",
                game.path);
        stop();
        return -1;
    }

    // Pre-flight memory check: sum ROM sizes and reject games that would exceed
    // the Vita's available memory. The Vita heap is 256 MB but the app, GPU, and
    // driver buffers consume a significant portion, leaving roughly 100 MB usable
    // for ROM data.  Drivers like Cave CV1000 also allocate large render buffers
    // (epic12 bitmap ~128 MB, palette LUT ~32 MB) on top of ROM data, so even a
    // modest ROM total can push the system past its limit.
    {
        UINT64 totalRomBytes = 0;
        struct BurnRomInfo ri;
        for (UINT32 i = 0; BurnDrvGetRomInfo(&ri, i) == 0; i++) {
            if (ri.nLen) {
                totalRomBytes += ri.nLen;
            }
        }

        // Estimate overhead multiplier based on hardware type.
        // CV1K games need ~2x ROM size for extra buffers (epic12 VRAM, palette
        // LUT, RAM copies, SH-3 emulation state).
        UINT64 estimatedTotal = totalRomBytes;
        int hardware = BurnDrvGetHardwareCode();
        if ((hardware & 0xffff0000) == HARDWARE_CAVE_CV1000) {
            estimatedTotal = totalRomBytes * 2;
        }

        // 200 MB is a safe upper bound for what we can allocate on Vita
        // (256 MB heap minus ~50 MB for app code, GPU, UI, audio, OS overhead).
        static const UINT64 VITA_MAX_EMU_BYTES = 200ULL * 1024 * 1024;

        printf("FBNeoVitaUiEmu::load: ROM data=%llu bytes, estimated total=%llu bytes (limit=%llu)\n",
               (unsigned long long)totalRomBytes, (unsigned long long)estimatedTotal,
               (unsigned long long)VITA_MAX_EMU_BYTES);

        if (estimatedTotal > VITA_MAX_EMU_BYTES) {
            printf("FBNeoVitaUiEmu::load: game requires too much memory\n");
            char detail[256];
            snprintf(detail, sizeof(detail),
                     "ROM data: %llu MB, estimated need: %llu MB\n"
                     "PS Vita limit: %llu MB",
                     (unsigned long long)(totalRomBytes / (1024 * 1024)),
                     (unsigned long long)(estimatedTotal / (1024 * 1024)),
                     (unsigned long long)(VITA_MAX_EMU_BYTES / (1024 * 1024)));
            fbneo_vita::load_error::set(
                    "Not Enough Memory",
                    "This game requires more memory than the\n"
                    "PS Vita can provide.",
                    detail);
            stop();
            return -1;
        }
    }

#ifdef __FBNEO_VITA_ARM__
    nSekCpuCore = getSekCpuCore();
    printf("nSekCpuCore: %s\n", nSekCpuCore > 0 ? "M68K" : "C68K (ASM)");
#endif

    int audio_freq = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_FREQ, true)->getInteger();
    nInterpolation = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_INTERPOLATION, true)->getInteger();
    nFMInterpolation = pMain->getConfig()->get(PEMUConfig::OptId::EMU_AUDIO_FMINTERPOLATION, true)->getInteger();
    bForce60Hz = pMain->getConfig()->get(PEMUConfig::OptId::EMU_FORCE_60HZ, true)->getInteger();
    if (bForce60Hz) {
        nBurnFPS = 6000;
    }

    ///////////////
    // FBA DRIVER
    ///////////////
    EnableHiscores = 1;

    printf("FBNeoVitaUiEmu::load: initialize driver...\n");
    // some drivers require audio buffer to be allocated for DrvInit, add a "dummy" one for now...
    auto *aud = new Audio(audio_freq);
    nBurnSoundRate = aud->getSampleRate();
    nBurnSoundLen = aud->getSamples();
    pBurnSoundOut = (INT16 *) malloc(aud->getSamplesSize());
    if (DrvInit((int) nBurnDrvActive, false) != 0) {
        printf("\nFBNeoVitaUiEmu::load: driver initialisation failed\n");
        delete (aud);
        if (!fbneo_vita::load_error::has()) {
            fbneo_vita::load_error::set(
                    "Driver Init Failed",
                    "FBNeo could not start this ROM.",
                    game.path);
        }
        stop();
        return -1;
    }
    delete (aud);
    free(pBurnSoundOut);
    pBurnSoundOut = nullptr;
    nFramesEmulated = 0;
    nFramesRendered = 0;
    nCurrentFrame = 0;
    ///////////////
    // FBA DRIVER
    ///////////////

    ///////////
    // AUDIO
    //////////
    addAudio(audio_freq, Audio::toSamples(audio_freq, (float) nBurnFPS / 100.0f));
    if (audio->isAvailable()) {
        nBurnSoundRate = audio->getSampleRate();
        nBurnSoundLen = audio->getSamples();
        pBurnSoundOut = (INT16 *) malloc(audio->getSamplesSize());
    }
    audio_sync = !bForce60Hz;
    targetFps = (float) nBurnFPS / 100.0f;
    printf("FBNeoVitaUiEmu::load: FORCE_60HZ: %i, AUDIO_SYNC: %i, FPS: %f\n", bForce60Hz, audio_sync, targetFps);
    ///////////
    // AUDIO
    //////////

    //////////
    // VIDEO
    //////////
    Vector2i size, aspect;
    BurnDrvGetFullSize(&size.x, &size.y);
    BurnDrvGetAspect(&aspect.x, &aspect.y);
    nBurnBpp = 2;
    BurnHighCol = myHighCol16;
    BurnRecalcPal();
    // video may already be initialized from fbneo driver (Reinitialise)
    if (!video) {
        auto v = new FBNeoVitaVideo(pMain, &pBurnDraw, &nBurnPitch, size, aspect);
        addVideo(v);
        printf("FBNeoVitaUiEmu::load: size: %i x %i, aspect: %i x %i, pitch: %i\n",
               size.x, size.y, aspect.x, aspect.y, nBurnPitch);
    } else {
        printf("FBNeoVitaUiEmu::load: video already initialized, skipped\n");
    }
    //////////
    // VIDEO
    //////////

    return UiEmu::load(game);
}

// need for some games
void Reinitialise(void) {
    Vector2i size, aspect;
    BurnDrvGetFullSize(&size.x, &size.y);
    BurnDrvGetAspect(&aspect.x, &aspect.y);
    auto v = new FBNeoVitaVideo(uiInstance, &pBurnDraw, &nBurnPitch, size, aspect);
    uiInstance->getUiEmu()->addVideo(v);
    printf("FBNeoVitaUiEmu::Reinitialise: size: %i x %i, aspect: %i x %i\n",
           size.x, size.y, aspect.x, aspect.y);
}

void ReinitialiseVideo(void) {
    Reinitialise();
}

void FBNeoVitaUiEmu::stop() {
    printf("FBNeoVitaUiEmu::stop()\n");
    DrvExit();
    if (pBurnSoundOut) {
        free(pBurnSoundOut);
        pBurnSoundOut = nullptr;
    }
    UiEmu::stop();
}

extern RguiMain *g_rgui;

bool FBNeoVitaUiEmu::onInput(c2d::Input::Player *players) {
    // if RGUI is visible, let it handle input
    if (g_rgui && g_rgui->isVisible()) {
        pMain->getInput()->setRotation(Input::Rotation::R0, Input::Rotation::R0);
        return g_rgui->onInput(players);
    }
    // rotation config:
    // 0 > "OFF"
    // 1 > "ON"
    // 2 > "FLIP"
    // 3 > "CAB" (vita/switch)
    int rotation = getUi()->getConfig()->get(PEMUConfig::OptId::EMU_ROTATION, true)->getArrayIndex();
    if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
        if (rotation == 0) {
            pMain->getInput()->setRotation(Input::Rotation::R90, Input::Rotation::R0);
        } else if (rotation == 1) {
            pMain->getInput()->setRotation(Input::Rotation::R0, Input::Rotation::R0);
        } else if (rotation == 2) {
            pMain->getInput()->setRotation(Input::Rotation::R270, Input::Rotation::R0);
        } else {
            pMain->getInput()->setRotation(Input::Rotation::R270, Input::Rotation::R270);
        }
    }

    // intercept menu combo to show RGUI instead of old menu
    if (((players[0].buttons & Input::Button::Menu1) && (players[0].buttons & Input::Button::Menu2))) {
        if (g_rgui) {
            pause();
            pMain->getInput()->setRotation(Input::Rotation::R0, Input::Rotation::R0);
            g_rgui->show(true);
            pMain->getInput()->clear();
            return true;
        }
    }

    // skip base UiEmu::onInput to avoid triggering old menu,
    // go directly to C2DObject::onInput
    return C2DObject::onInput(players);
}

void FBNeoVitaUiEmu::onUpdate() {
    if (isPaused() || !video || !bDrvOkay) {
        return;
    }

    // update fbneo inputs
    InputMake(true);

    // handle diagnostic and reset switch
    unsigned int buttons = pMain->getInput()->getButtons();
    if (buttons & Input::Button::Select) {
        if (clock.getElapsedTime().asSeconds() > 2) {
            if (pgi_reset) {
                pMain->getUiStatusBox()->show("TIPS: PRESS START "
                                              "BUTTON 2 SECONDS FOR DIAG MENU...");
                pgi_reset->Input.nVal = 1;
                *(pgi_reset->Input.pVal) = pgi_reset->Input.nVal;
            }
            nCurrentFrame = 0;
            nFramesEmulated = 0;
            clock.restart();
        }
    } else if (buttons & Input::Button::Start) {
        if (clock.getElapsedTime().asSeconds() > 2) {
            if (pgi_diag) {
                pMain->getUiStatusBox()->show("TIPS: PRESS COIN "
                                              "BUTTON 2 SECONDS TO RESET CURRENT GAME...");
                pgi_diag->Input.nVal = 1;
                *(pgi_diag->Input.pVal) = pgi_diag->Input.nVal;
            }
            clock.restart();
        }
    } else {
        clock.restart();
    }

    // update fbneo video buffer and audio
#ifdef __VITA__
    int skip = pMain->getConfig()->get(PEMUConfig::OptId::EMU_FRAMESKIP, true)->getInteger();
#else
    int skip = 0;
#endif

    pBurnDraw = nullptr;
    frameskip++;

    if (frameskip > skip) {
        video->getTexture()->lock(&pBurnDraw, &nBurnPitch);
        nFramesRendered++;
    }

    BurnDrvFrame();
    nCurrentFrame++;

    if (frameskip > skip) {
        video->getTexture()->unlock();
        frameskip = 0;
    }

    if (audio) {
#if 0
        int queued = audio->getSampleBufferQueued();
        int capacity = audio->getSampleBufferCapacity();
        if (audio->getSamples() + queued > capacity) {
            printf("WARNING: samples: %i, queued: %i, capacity: %i (fps: %f)\n",
                   audio->getSamples(), queued, capacity, targetFps);
        }
#endif
        audio->play(pBurnSoundOut, audio->getSamples(),
                    audio_sync ? Audio::SyncMode::LowLatency : Audio::SyncMode::None);
    }

    UiEmu::onUpdate();
}
