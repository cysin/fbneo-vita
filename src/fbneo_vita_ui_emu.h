//
// Created by cpasjuste on 01/06/18.
//

#ifndef FBNEO_VITA_UI_EMU_H
#define FBNEO_VITA_UI_EMU_H

#include <string>

#include "runtime/ui_emu.h"

class FBNeoVitaUiEmu : public pemu::UiEmu {
public:
    explicit FBNeoVitaUiEmu(pemu::UiMain *ui);

    int load(const pemu::Game &game) override;

    void stop() override;

private:
#ifdef __FBNEO_VITA_ARM__
    int getSekCpuCore();
#endif

    bool onInput(c2d::Input::Player *players) override;

    void onUpdate() override;

    bool audio_sync = false;
    int frameskip = 0;
    c2d::C2DClock clock;
};

#endif //FBNEO_VITA_UI_EMU_H
