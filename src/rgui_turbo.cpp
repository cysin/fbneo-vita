//
// RGUI turbo fire settings
//

#include "rgui_turbo.h"
#include <cstdio>
#include <cstring>

using namespace c2d;

TurboConfig g_turbo;
int g_turbo_frame_counter = 0;

namespace {
    constexpr const char *GLOBAL_TURBO_FILE = "global_turbo.cfg";
    constexpr int DISPLAY_ORDER[TURBO_MAX_BUTTONS] = {
        0, // Cross
        1, // Circle
        4, // Square
        5, // Triangle
        2, // L1
        3, // R1
        6, // L2
        7, // R2
    };

    void resetTurboConfig() {
        memset(&g_turbo.enabled, 0, sizeof(g_turbo.enabled));
        g_turbo.speed = 2;
    }

    void saveTurboConfig(FILE *f) {
        fprintf(f, "speed=%d\n", g_turbo.speed);
        for (int p = 0; p < TURBO_MAX_PLAYERS; p++) {
            for (int b = 0; b < TURBO_MAX_BUTTONS; b++) {
                if (g_turbo.enabled[p][b]) {
                    fprintf(f, "%d,%d=1\n", p, b);
                }
            }
        }
    }

    void loadTurboConfig(FILE *f) {
        char line[64];
        while (fgets(line, sizeof(line), f)) {
            int val;
            if (sscanf(line, "speed=%d", &val) == 1) {
                if (val >= 1 && val <= 10) g_turbo.speed = val;
            } else {
                int p, b;
                if (sscanf(line, "%d,%d=%d", &p, &b, &val) == 3) {
                    if (p >= 0 && p < TURBO_MAX_PLAYERS && b >= 0 && b < TURBO_MAX_BUTTONS) {
                        g_turbo.enabled[p][b] = (val != 0);
                    }
                }
            }
        }
    }
}

static const char *s_button_names[TURBO_MAX_BUTTONS] = {
    "Cross", "Circle", "L1", "R1", "Square", "Triangle", "L2", "R2"
};

RguiTurbo::RguiTurbo(Renderer *renderer, Font *font)
    : m_renderer(renderer) {
    memset(&g_turbo, 0, sizeof(g_turbo));
    g_turbo.speed = 2;
    m_menu = new RguiMenu(renderer, font, "Turbo Fire", {});
    refresh();
}

RguiTurbo::~RguiTurbo() {
    delete m_menu;
}

const char *RguiTurbo::getButtonName(int btn) {
    if (btn >= 0 && btn < TURBO_MAX_BUTTONS) return s_button_names[btn];
    return "?";
}

void RguiTurbo::refresh() {
    std::vector<RguiMenuItem> items;
    int id = 0;

    for (int p = 0; p < TURBO_MAX_PLAYERS; p++) {
        for (int di = 0; di < TURBO_MAX_BUTTONS; di++) {
            const int b = DISPLAY_ORDER[di];
            RguiMenuItem item;
            char label[32];
            snprintf(label, sizeof(label), "P%d %s", p + 1, s_button_names[b]);
            item.label = label;
            item.value = g_turbo.enabled[p][b] ? "ON" : "OFF";
            item.id = id++;
            items.push_back(item);
        }
    }

    // speed setting
    RguiMenuItem speedItem;
    speedItem.label = "Turbo Speed";
    char speedStr[16];
    snprintf(speedStr, sizeof(speedStr), "%d frames", g_turbo.speed);
    speedItem.value = speedStr;
    speedItem.id = id++;
    items.push_back(speedItem);

    m_menu->setItems(items);
}

int RguiTurbo::handleInput(Input *input) {
    auto action = m_menu->handleInput(input);

    if (action == RguiMenu::CANCEL) {
        return 1;
    }

    if (action == RguiMenu::LEFT || action == RguiMenu::RIGHT ||
        action == RguiMenu::CONFIRM) {
        int sel = m_menu->getSelectedIndex();
        int totalButtons = TURBO_MAX_PLAYERS * TURBO_MAX_BUTTONS;

        if (sel < totalButtons) {
            // toggle button turbo
            int player = sel / TURBO_MAX_BUTTONS;
            int displayButton = sel % TURBO_MAX_BUTTONS;
            int button = DISPLAY_ORDER[displayButton];
            g_turbo.enabled[player][button] = !g_turbo.enabled[player][button];
        } else if (sel == totalButtons) {
            // adjust speed
            if (action == RguiMenu::RIGHT || action == RguiMenu::CONFIRM) {
                g_turbo.speed++;
                if (g_turbo.speed > 10) g_turbo.speed = 1;
            } else {
                g_turbo.speed--;
                if (g_turbo.speed < 1) g_turbo.speed = 10;
            }
        }

        refresh();
        m_menu->setSelectedIndex(sel);
    }

    return -1;
}

void RguiTurbo::save(const char *driverName) {
    if (!driverName || !driverName[0]) return;

    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s_turbo.cfg", szAppConfigPath, driverName);

    FILE *f = fopen(path, "w");
    if (!f) return;
    saveTurboConfig(f);
    fclose(f);
}

void RguiTurbo::saveGlobal() {
    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s", szAppConfigPath, GLOBAL_TURBO_FILE);

    FILE *f = fopen(path, "w");
    if (!f) return;
    saveTurboConfig(f);
    fclose(f);
}

void RguiTurbo::loadGlobal() {
    resetTurboConfig();

    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s", szAppConfigPath, GLOBAL_TURBO_FILE);

    FILE *f = fopen(path, "r");
    if (!f) return;
    loadTurboConfig(f);
    fclose(f);
}

void RguiTurbo::load(const char *driverName) {
    loadGlobal();
    if (!driverName || !driverName[0]) return;

    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s_turbo.cfg", szAppConfigPath, driverName);

    FILE *f = fopen(path, "r");
    if (!f) return;
    loadTurboConfig(f);
    fclose(f);
}

void RguiTurbo::draw(Transform &t) {
    m_menu->onDraw(t, true);
}
