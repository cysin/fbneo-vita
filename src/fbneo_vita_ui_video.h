//
// Created by cpasjuste on 25/11/16.
//

#ifndef FBNEO_VITA_VIDEO_H
#define FBNEO_VITA_VIDEO_H

#include "runtime/ui_video.h"

namespace pemu {
    class FBNeoVitaVideo : public C2DUIVideo {
    public:
        FBNeoVitaVideo(UiMain *ui, uint8_t **pixels, int *pitch, const c2d::Vector2i &size,
                  const c2d::Vector2i &aspect = {4, 3});

        void updateScaling(bool vertical = false, bool flip = false) override;
    };
}

#endif //FBNEO_VITA_VIDEO_H
