//
// Created by cpasjuste on 29/05/18.
//

#ifndef FBNEO_VITA_CONFIG_H
#define FBNEO_VITA_CONFIG_H

#include "runtime/pemu_config.h"
#include "fbneo_vita_utility.h"
#include "burner.h"

class FBNeoVitaConfig final : public pemu::PEMUConfig {
public:
    FBNeoVitaConfig(c2d::Renderer *renderer, int version);

    std::vector<int> getCoreHiddenOptionToEnable() override {
        return {
                UI_FILTER_CLONES,
                UI_FILTER_SYSTEM
        };
    }

    std::string getCoreVersion() override {
        return "fbneo: " + std::string(szAppBurnVer);
    }

    std::vector<std::string> getCoreSupportedExt() override {
        return {".zip"};
    }
};

#endif //FBNEO_VITA_CONFIG_H
