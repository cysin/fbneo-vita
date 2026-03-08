//
// Created by cpasjuste on 28/09/23.
//

#ifndef FBNEO_VITA_UTILITY_H
#define FBNEO_VITA_UTILITY_H

#include <string>

#include "runtime/game_types.h"

#define HARDWARE_PREFIX_ARCADE 0x12341234

class FBNeoVitaUtility {
public:
    struct GameInfo {
        std::string name;
        std::string manufacturer;
        std::string parent;
        std::string sysName;
        int sysId;
        int players;
    };

    static int setDriverActive(const pemu::Game &game);

    static GameInfo getGameInfo(const pemu::Game &game);
};

#endif //FBNEO_VITA_UTILITY_H
