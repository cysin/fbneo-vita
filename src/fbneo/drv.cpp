// Driver Init module

#include "runtime/runtime.h"
#include "burner.h"
#include "retro_input.h"
#include "rgui_turbo.h"
#include "rgui_cheats.h"

using namespace pemu;

extern UiMain *pemu_ui;
extern StringSet BzipText;
extern StringSet BzipDetail;

extern UINT8 NeoSystem;
int bDrvOkay = 0;
int kNetGame = 0;
INT32 nInputIntfMouseDivider = 1;
bool bRunPause;

bool is_netgame_or_recording() {
    return false;
}

static int ProgressCreate();

static UINT8 NeoSystemList[] = {
        0x13, // "Universe BIOS ver. 4.0"
        0x14, // "Universe BIOS ver. 3.3"
        0x15, // "Universe BIOS ver. 3.2"
        0x16, // "Universe BIOS ver. 3.1"
        0x00, // "MVS Asia/Europe ver. 6 (1 slot)"
        0x01, // "MVS Asia/Europe ver. 5 (1 slot)"
        0x02, // "MVS Asia/Europe ver. 3 (4 slot)"
        0x03, // "MVS USA ver. 5 (2 slot)"
        0x04, // "MVS USA ver. 5 (4 slot)"
        0x05, // "MVS USA ver. 5 (6 slot)"
        0x08, // "MVS Japan ver. 6 (? slot)"
        0x09, // "MVS Japan ver. 5 (? slot)"
        0x0a, // "MVS Japan ver. 3 (4 slot)"
        0x0d, // "MVS Japan (J3)"
        0x10, // "AES Asia"
        0x0f, // "AES Japan"
        0x0b, // "NEO-MVH MV1C (Asia)"
        0x0c, // "NEO-MVH MV1C (Japan)"
        0x12, // "Deck ver. 6 (Git Ver 1.3)"
        0x11, // "Development Kit"
};
static_assert(sizeof(NeoSystemList) / sizeof(NeoSystemList[0]) == 20,
              "NeoSystemList must match the NEOBIOS option list");

static UINT8 getNeoSystemFromConfig() {
    constexpr int kDefaultNeoBiosIndex = 4; // MVS Asia/Europe ver. 6 (1 slot)
    const auto opt = pemu_ui->getConfig()->get(PEMUConfig::OptId::EMU_NEOBIOS, true);
    int index = opt ? opt->getArrayIndex() : kDefaultNeoBiosIndex;
    const int count = (int)(sizeof(NeoSystemList) / sizeof(NeoSystemList[0]));

    if (index < 0 || index >= count) {
        printf("NeoSystem: invalid BIOS index %i, fallback to %i\n", index, kDefaultNeoBiosIndex);
        index = kDefaultNeoBiosIndex;
    }

    return NeoSystemList[index];
}

static int DoLibInit() {
    int nRet;

    ProgressCreate();

    nRet = BzipOpen(false);
    printf("DoLibInit: BzipOpen = %i\n", nRet);
    if (nRet) {
        if (!fbneo_vita::load_error::has()) {
            std::string summary = BzipText.szText ? BzipText.szText : "The ROM set could not be opened.";
            std::string detail = BzipDetail.szText ? BzipDetail.szText : "";
            fbneo_vita::load_error::set("ROM Load Failed", summary, detail);
        }
        BzipClose();
        return 1;
    }

    NeoSystem = getNeoSystemFromConfig();
    printf("DoLibInit: NeoSystem = 0x%02x\n", NeoSystem);

    nRet = BurnDrvInit();
    printf("DoLibInit: BurnDrvInit = %i\n", nRet);

    BzipClose();

    if (nRet) {
        BurnDrvExit();
        return 1;
    } else {
        return 0;
    }
}

// Catch calls to BurnLoadRom() once the emulation has started;
// Intialise the zip module before forwarding the call, and exit cleanly.
static int DrvLoadRom(unsigned char *Dest, int *pnWrote, int i) {
    int nRet;

    BzipOpen(false);

    if ((nRet = BurnExtLoadRom(Dest, pnWrote, i)) != 0) {
        char szText[256];
        char *pszFilename;
        BurnDrvGetRomName(&pszFilename, i, 0);
        sprintf(szText, "Error loading %s for %s.\nEmulation will likely have problems.",
                pszFilename, BurnDrvGetTextA(DRV_NAME));
        printf("DrvLoadRom: %s\n", szText);
        fbneo_vita::load_error::set(
                "ROM Load Failed",
                "A required ROM entry could not be loaded.",
                szText);
    }

    BzipClose();

    BurnExtLoadRom = DrvLoadRom;

    return nRet;
}

int DrvInit(int nDrvNum, bool bRestore) {
    printf("DrvInit(%i, %i)\n", nDrvNum, bRestore);
    DrvExit();

    // set selected driver
    nBurnDrvSelect[0] = (UINT32) nDrvNum;
    // for retro_input
    bIsNeogeoCartGame = ((BurnDrvGetHardwareCode() & HARDWARE_PUBLIC_MASK) == HARDWARE_SNK_NEOGEO);
    // default input values
    nMaxPlayers = BurnDrvGetMaxPlayers();
    SetDefaultDeviceTypes();
    // init inputs
    InputInit();
    SetControllerInfo();

    printf("DrvInit: DoLibInit()\n");
    if (DoLibInit()) {                // Init the Burn library's driver
        //char szTemp[512];
        //_stprintf(szTemp, _T("Error starting '%s'.\n"), BurnDrvGetText(DRV_FULLNAME));
        //AppError(szTemp, 1);
        return 1;
    }

    printf("DrvInit: BurnExtLoadRom = DrvLoadRom\n");
    BurnExtLoadRom = DrvLoadRom;

    char path[1024];
    snprintf(path, 1023, "%s%s.fs", szAppEEPROMPath, BurnDrvGetTextA(DRV_NAME));
    BurnStateLoad(path, 0, nullptr);

    bDrvOkay = 1;
    nBurnLayer = 0xff;
    nSpriteEnable = 0xff;

    // load cheats
    bCheatsAllowed = true;
    ConfigCheatLoad();

    // load per-game turbo and cheat settings
    {
        const char *drvName = BurnDrvGetTextA(DRV_NAME);
        RguiTurbo::load(drvName);
        RguiCheats::loadState(drvName);
    }

    return 0;
}

// for uiStateMenu.cpp (BurnStateLoad)
int DrvInitCallback() {
    return DrvInit((int) nBurnDrvSelect[0], false);
}

int DrvExit() {
    if (bDrvOkay) {
        printf("DrvExit: begin\n");
        // save per-game turbo and cheat settings before exit
        {
            const char *drvName = BurnDrvGetTextA(DRV_NAME);
            printf("DrvExit: save turbo/cheats for %s\n", drvName ? drvName : "(null)");
            RguiTurbo::save(drvName);
            RguiCheats::saveState(drvName);
        }
        if (nBurnDrvSelect[0] < nBurnDrvCount) {
            char path[1024];
            snprintf(path, 1023, "%s%s.fs", szAppEEPROMPath, BurnDrvGetTextA(DRV_NAME));
            printf("DrvExit: BurnStateSave(%s)\n", path);
            BurnStateSave(path, 0);
            printf("DrvExit: InputExit()\n");
            InputExit();
            printf("DrvExit: BurnDrvExit()\n");
            BurnDrvExit();
        }
    }

    printf("DrvExit: cleanup globals\n");
    BurnExtLoadRom = nullptr;
    bDrvOkay = 0;
    nBurnDrvSelect[0] = ~0U;

    return 0;
}

static double nProgressPosBurn = 0;

static int ProgressCreate() {
    nProgressPosBurn = 0;
    pemu_ui->getUiProgressBox()->setVisibility(c2d::Visibility::Visible);
    pemu_ui->getUiProgressBox()->setLayer(1000);
    return 0;
}

int ProgressUpdateBurner(double dProgress, const TCHAR *pszText, bool bAbs) {
    pemu_ui->getUiProgressBox()->setTitle(BurnDrvGetTextA(DRV_FULLNAME));

    if (pszText) {
        nProgressPosBurn += dProgress;
        pemu_ui->getUiProgressBox()->setMessage(pszText);
        pemu_ui->getUiProgressBox()->setProgress((float) nProgressPosBurn);
    } else {
        pemu_ui->getUiProgressBox()->setMessage("Please wait...");
    }

    pemu_ui->flip();

    return 0;
}

int AppError(TCHAR *szText, int bWarning) {
    //ui->getUiMessageBox()->show("ERROR", szText ? szText : "UNKNOW ERROR", "OK");
    return 1;
}

#ifdef __FBNEO_VITA_LIGHT__

void nes_add_cheat(char *code) {};

void nes_remove_cheat(char *code) {};
#endif
