//
// RGUI cheats menu
//

#include "rgui_cheats.h"
#include "burner.h"
#include <cstdio>

using namespace c2d;

RguiCheats::RguiCheats(Renderer *renderer, Font *font)
    : m_renderer(renderer) {
    m_menu = new RguiMenu(renderer, font, "Cheats", {});
}

RguiCheats::~RguiCheats() {
    delete m_menu;
}

static bool isBlankName(const char *name) {
    if (!name) return true;
    while (*name) {
        if (*name != ' ' && *name != '\t') return false;
        name++;
    }
    return true;
}

void RguiCheats::refresh() {
    std::vector<RguiMenuItem> items;
    m_cheat_count = 0;

    if (!pCheatInfo) {
        RguiMenuItem item;
        item.label = "No cheats available";
        item.id = -1;
        items.push_back(item);
        m_menu->setItems(items);
        return;
    }

    CheatInfo *pci = pCheatInfo;
    int idx = 0;
    while (pci) {
        RguiMenuItem item;
        item.label = pci->szCheatName;
        item.id = idx;

        if (isBlankName(pci->szCheatName)) {
            // empty cheat: show blank line, no value
            item.label = " ";
            item.value = "";
        } else if (pci->nCurrent >= 0 && pci->pOption[pci->nCurrent]) {
            item.value = pci->pOption[pci->nCurrent]->szOptionName;
        } else {
            item.value = "OFF";
        }

        items.push_back(item);
        pci = pci->pNext;
        idx++;
        m_cheat_count++;
    }

    m_menu->setItems(items);
}

int RguiCheats::handleInput(Input *input) {
    auto action = m_menu->handleInput(input);

    if (action == RguiMenu::CANCEL) {
        return 1;
    }

    if (m_cheat_count == 0) return -1;

    if (action == RguiMenu::LEFT || action == RguiMenu::RIGHT ||
        action == RguiMenu::CONFIRM) {
        int sel = m_menu->getSelectedIndex();
        if (sel < 0 || sel >= m_cheat_count) return -1;

        // find the cheat at index sel
        CheatInfo *pci = pCheatInfo;
        for (int i = 0; i < sel && pci; i++) {
            pci = pci->pNext;
        }
        if (!pci) return -1;

        // skip blank cheats
        if (isBlankName(pci->szCheatName)) return -1;

        // count options
        int nOptions = 0;
        for (int i = 0; i < CHEAT_MAX_OPTIONS; i++) {
            if (pci->pOption[i]) nOptions++;
            else break;
        }
        if (nOptions == 0) return -1;

        int current = pci->nCurrent;
        if (action == RguiMenu::RIGHT || action == RguiMenu::CONFIRM) {
            current++;
            if (current >= nOptions) current = 0;
        } else {
            current--;
            if (current < 0) current = nOptions - 1;
        }

        CheatEnable(sel, current);
        refresh();
        m_menu->setSelectedIndex(sel);
    }

    return -1;
}

void RguiCheats::saveState(const char *driverName) {
    if (!driverName || !driverName[0]) return;
    if (!pCheatInfo) return;

    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s_cheats.cfg", szAppConfigPath, driverName);

    FILE *f = fopen(path, "w");
    if (!f) return;

    CheatInfo *pci = pCheatInfo;
    int idx = 0;
    while (pci) {
        if (pci->nCurrent != pci->nDefault) {
            fprintf(f, "%d=%d\n", idx, pci->nCurrent);
        }
        pci = pci->pNext;
        idx++;
    }

    fclose(f);
}

void RguiCheats::loadState(const char *driverName) {
    if (!driverName || !driverName[0]) return;
    if (!pCheatInfo) return;

    extern char szAppConfigPath[];
    char path[1024];
    snprintf(path, sizeof(path), "%s%s_cheats.cfg", szAppConfigPath, driverName);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int idx, option;
        if (sscanf(line, "%d=%d", &idx, &option) == 2) {
            CheatEnable(idx, option);
        }
    }

    fclose(f);
}

void RguiCheats::draw(Transform &t) {
    m_menu->onDraw(t, true);
}
