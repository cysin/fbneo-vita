#ifndef FBNEO_VITA_MAIN_H
#define FBNEO_VITA_MAIN_H

#include "runtime/runtime.h"
#include "fbneo_vita_ui_emu.h"
#include "fbneo_vita_config.h"
#include "fbneo_vita_io.h"

#define PEMUIo FBNeoVitaIo
#define PEMUConfig FBNeoVitaConfig
#define PEMUSkin pemu::Skin
#define PEMUUiMain pemu::UiMain
#define PEMUUiEmu FBNeoVitaUiEmu

#endif
