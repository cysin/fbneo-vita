//
// RGUI cheats menu
//

#ifndef FBNEO_VITA_RGUI_CHEATS_H
#define FBNEO_VITA_RGUI_CHEATS_H

#include "rgui_menu.h"
#include "cross2d/c2d.h"

class RguiCheats {
public:
    RguiCheats(c2d::Renderer *renderer, c2d::Font *font);
    ~RguiCheats();

    void refresh();

    // returns: 0=done, 1=cancelled, -1=navigating
    int handleInput(c2d::Input *input);

    void draw(c2d::Transform &t);

    static void saveState(const char *driverName);
    static void loadState(const char *driverName);

private:
    c2d::Renderer *m_renderer;
    RguiMenu *m_menu;
    int m_cheat_count = 0;
};

#endif // FBNEO_VITA_RGUI_CHEATS_H
