#ifndef FBNEO_VITA_IO_H
#define FBNEO_VITA_IO_H

#include "burner.h"

extern void BurnPathsInit(C2DIo *io);

namespace c2d {
    class FBNeoVitaIo : public c2d::C2DIo {
    public:
        FBNeoVitaIo() : C2DIo() {
            C2DIo::create(FBNeoVitaIo::getDataPath());
            C2DIo::create(FBNeoVitaIo::getDataPath() + "configs");
            C2DIo::create(FBNeoVitaIo::getDataPath() + "saves");
            BurnPathsInit(this);
            BurnLibInit();
        }

        ~FBNeoVitaIo() override {
            printf("~FBNeoVitaIo()\n");
            BurnLibExit();
        }

        std::string getDataPath() override {
            return "ux0:/data/fbneo-vita/";
        }
    };
}

#endif
