//
// RGUI controls menu
//

#include "rgui_controls.h"
#include "fbneo/retro_input_wrapper.h"
#include "runtime/runtime.h"
#include <algorithm>
#include <cstdio>

using namespace c2d;
using namespace pemu;

namespace {
    struct PhysicalItem {
        int code;
        const char *label;
    };

    struct ActionSlot {
        int opt_id;
        const char *fallback_label;
        unsigned retro_id;
    };

    constexpr PhysicalItem PHYSICAL_ITEMS[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, "Up"},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, "Down"},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, "Left"},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "Right"},
        {SDL_CONTROLLER_BUTTON_A, "Cross"},
        {SDL_CONTROLLER_BUTTON_B, "Circle"},
        {SDL_CONTROLLER_BUTTON_X, "Square"},
        {SDL_CONTROLLER_BUTTON_Y, "Triangle"},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "L1"},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "R1"},
        {SDL_CONTROLLER_AXIS_TRIGGERLEFT + 100, "L2"},
        {SDL_CONTROLLER_AXIS_TRIGGERRIGHT + 100, "R2"},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, "L3"},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, "R3"},
        {SDL_CONTROLLER_BUTTON_BACK, "Select"},
        {SDL_CONTROLLER_BUTTON_START, "Start"},
    };

    constexpr ActionSlot ACTION_SLOTS[] = {
        {PEMUConfig::OptId::JOY_UP, "Up", RETRO_DEVICE_ID_JOYPAD_UP},
        {PEMUConfig::OptId::JOY_DOWN, "Down", RETRO_DEVICE_ID_JOYPAD_DOWN},
        {PEMUConfig::OptId::JOY_LEFT, "Left", RETRO_DEVICE_ID_JOYPAD_LEFT},
        {PEMUConfig::OptId::JOY_RIGHT, "Right", RETRO_DEVICE_ID_JOYPAD_RIGHT},
        {PEMUConfig::OptId::JOY_A, "Button 1", RETRO_DEVICE_ID_JOYPAD_B},
        {PEMUConfig::OptId::JOY_B, "Button 2", RETRO_DEVICE_ID_JOYPAD_A},
        {PEMUConfig::OptId::JOY_X, "Button 3", RETRO_DEVICE_ID_JOYPAD_Y},
        {PEMUConfig::OptId::JOY_Y, "Button 4", RETRO_DEVICE_ID_JOYPAD_X},
        {PEMUConfig::OptId::JOY_LT, "Button 5", RETRO_DEVICE_ID_JOYPAD_L},
        {PEMUConfig::OptId::JOY_RT, "Button 6", RETRO_DEVICE_ID_JOYPAD_R},
        {PEMUConfig::OptId::JOY_LB, "Button 7", RETRO_DEVICE_ID_JOYPAD_L2},
        {PEMUConfig::OptId::JOY_RB, "Button 8", RETRO_DEVICE_ID_JOYPAD_R2},
        {PEMUConfig::OptId::JOY_LS, "Button 9", RETRO_DEVICE_ID_JOYPAD_L3},
        {PEMUConfig::OptId::JOY_RS, "Button 10", RETRO_DEVICE_ID_JOYPAD_R3},
        {PEMUConfig::OptId::JOY_SELECT, "Coin / Select", RETRO_DEVICE_ID_JOYPAD_SELECT},
        {PEMUConfig::OptId::JOY_START, "Start", RETRO_DEVICE_ID_JOYPAD_START},
    };

    constexpr int ROW_ID_DEADZONE = -1000;
    constexpr int ACTION_PICKER_ID_UNBOUND = -2000;

    bool isTriggerAxis(int value) {
        return value == (SDL_CONTROLLER_AXIS_TRIGGERLEFT + 100) ||
               value == (SDL_CONTROLLER_AXIS_TRIGGERRIGHT + 100);
    }

    std::string joinLabels(const std::vector<std::string> &labels) {
        if (labels.empty()) {
            return "Unbound";
        }

        std::string joined = labels[0];
        for (size_t i = 1; i < labels.size(); i++) {
            if (joined.size() + labels[i].size() + 3 > 64) {
                joined += " / ...";
                break;
            }
            joined += " / " + labels[i];
        }
        return joined;
    }

    const ActionSlot *findActionSlot(int opt_id) {
        for (const auto &slot : ACTION_SLOTS) {
            if (slot.opt_id == opt_id) {
                return &slot;
            }
        }
        return nullptr;
    }
}

RguiControls::RguiControls(UiMain *ui, Renderer *renderer, Font *font)
    : m_ui(ui), m_renderer(renderer) {
    m_menu = new RguiMenu(renderer, font, "Default Controls", {});
    m_action_picker = new RguiMenu(renderer, font, "Select Action", {});
}

RguiControls::~RguiControls() {
    delete m_action_picker;
    delete m_menu;
}

void RguiControls::refresh(bool in_game) {
    m_in_game = in_game;

    std::vector<RguiMenuItem> items;
    items.reserve(sizeof(PHYSICAL_ITEMS) / sizeof(PHYSICAL_ITEMS[0]) + 1);
    for (const auto &item : PHYSICAL_ITEMS) {
        items.push_back({item.label, getBoundActionLabel(item.code), item.code, false});
    }
    items.push_back({"Dead Zone", getOptionValue(PEMUConfig::OptId::JOY_DEADZONE), ROW_ID_DEADZONE, false});

    m_menu->setItems(items);
    m_menu->setTitle(in_game ? "Controls - This Game" : "Default Controls");
}

void RguiControls::cycleDeadZone(int direction) {
    auto *opt = m_ui->getConfig()->get(PEMUConfig::OptId::JOY_DEADZONE, m_in_game);
    if (!opt) {
        return;
    }

    int idx = opt->getArrayIndex() + direction;
    int count = (int)opt->getArray().size();
    if (count <= 0) {
        return;
    }
    if (idx >= count) {
        idx = 0;
    } else if (idx < 0) {
        idx = count - 1;
    }

    opt->setArrayIndex(idx);
    saveCurrentScope();
}

void RguiControls::openActionPicker(int physical_code) {
    m_picker_physical_code = physical_code;
    m_picker_visible = true;
    buildActionPicker(physical_code);
    m_ui->getInput()->clear();
}

void RguiControls::buildActionPicker(int physical_code) {
    std::vector<RguiMenuItem> items;
    items.reserve(sizeof(ACTION_SLOTS) / sizeof(ACTION_SLOTS[0]) + 1);
    items.push_back({"Unbound", "", ACTION_PICKER_ID_UNBOUND, false});

    std::vector<const ActionSlot *> picker_slots;
    for (const auto &slot : ACTION_SLOTS) {
        if (!m_in_game || isActionAvailable(slot.opt_id)) {
            picker_slots.push_back(&slot);
        }
    }
    if (picker_slots.empty()) {
        for (const auto &slot : ACTION_SLOTS) {
            picker_slots.push_back(&slot);
        }
    }

    const int current_action = getPrimaryBoundActionOptId(physical_code, true);
    int selected_index = 0;

    for (const auto *slot : picker_slots) {
        auto *opt = m_ui->getConfig()->get(slot->opt_id, m_in_game);
        items.push_back({
            getActionLabel(slot->opt_id),
            opt ? getBindingLabel(opt->getInteger()) : "Unbound",
            slot->opt_id,
            false
        });

        if (slot->opt_id == current_action) {
            selected_index = (int)items.size() - 1;
        }
    }

    m_action_picker->setItems(items);
    m_action_picker->setTitle("Map " + getBindingLabel(physical_code));
    m_action_picker->setSelectedIndex(selected_index);
}

void RguiControls::applyPhysicalBinding(int physical_code, int action_opt_id) {
    auto clearPhysicalExcept = [&](int code, int keep_a, int keep_b) {
        bool local_changed = false;
        for (const auto &slot : ACTION_SLOTS) {
            if (slot.opt_id == keep_a || slot.opt_id == keep_b) {
                continue;
            }

            auto *opt = m_ui->getConfig()->get(slot.opt_id, m_in_game);
            if (!opt) {
                continue;
            }

            if (opt->getInteger() == code) {
                opt->setInteger(SDL_CONTROLLER_BUTTON_INVALID);
                local_changed = true;
            }
        }
        return local_changed;
    };

    bool changed = false;
    const int current_action = getPrimaryBoundActionOptId(physical_code, true);

    if (action_opt_id == ACTION_PICKER_ID_UNBOUND) {
        changed |= clearPhysicalExcept(physical_code, ACTION_PICKER_ID_UNBOUND, ACTION_PICKER_ID_UNBOUND);
    } else {
        auto *target = m_ui->getConfig()->get(action_opt_id, m_in_game);
        if (!target) {
            return;
        }

        const int previous_physical = target->getInteger();

        changed |= clearPhysicalExcept(physical_code, action_opt_id, ACTION_PICKER_ID_UNBOUND);

        if (target->getInteger() != physical_code) {
            target->setInteger(physical_code);
            changed = true;
        }

        if (current_action != ACTION_PICKER_ID_UNBOUND && current_action != action_opt_id) {
            auto *current = m_ui->getConfig()->get(current_action, m_in_game);
            if (current) {
                const int swap_code =
                    previous_physical == physical_code ? SDL_CONTROLLER_BUTTON_INVALID : previous_physical;
                if (current->getInteger() != swap_code) {
                    current->setInteger(swap_code);
                    changed = true;
                }
            }
        }

        if (previous_physical != SDL_CONTROLLER_BUTTON_INVALID && previous_physical != physical_code) {
            changed |= clearPhysicalExcept(previous_physical, current_action, ACTION_PICKER_ID_UNBOUND);
        }
    }

    if (changed) {
        saveCurrentScope();
    }
}

void RguiControls::saveCurrentScope() const {
    if (m_in_game) {
        m_ui->getConfig()->saveGame();
    } else {
        m_ui->getConfig()->save();
    }

    // Keep menu navigation on the UI mapping while the controls screen is open.
    // The per-game control mapping is restored when the emulator resumes.
    m_ui->updateInputMapping(false);
}

std::string RguiControls::getRuntimeActionLabel(int opt_id) const {
    const auto *slot = findActionSlot(opt_id);
    if (!slot) {
        return {};
    }

    if (!m_in_game) {
        return {};
    }

    return FBNeoGetJoypadActionLabel(slot->retro_id, 0);
}

bool RguiControls::isActionAvailable(int opt_id) const {
    return !getRuntimeActionLabel(opt_id).empty();
}

std::string RguiControls::getActionLabel(int opt_id) const {
    std::string runtime_label = getRuntimeActionLabel(opt_id);
    if (!runtime_label.empty()) {
        return runtime_label;
    }

    const auto *slot = findActionSlot(opt_id);
    if (!slot) {
        return "Unknown";
    }
    return slot->fallback_label;
}

std::string RguiControls::getBoundActionLabel(int physical_code) const {
    std::vector<std::string> labels;

    for (const auto &slot : ACTION_SLOTS) {
        auto *opt = m_ui->getConfig()->get(slot.opt_id, m_in_game);
        if (!opt || opt->getInteger() != physical_code) {
            continue;
        }

        std::string label = m_in_game ? getRuntimeActionLabel(slot.opt_id) : getActionLabel(slot.opt_id);
        if (label.empty()) {
            continue;
        }
        if (std::find(labels.begin(), labels.end(), label) == labels.end()) {
            labels.push_back(label);
        }
    }

    return joinLabels(labels);
}

int RguiControls::getPrimaryBoundActionOptId(int physical_code, bool available_only) const {
    for (const auto &slot : ACTION_SLOTS) {
        auto *opt = m_ui->getConfig()->get(slot.opt_id, m_in_game);
        if (!opt || opt->getInteger() != physical_code) {
            continue;
        }
        if (available_only && m_in_game && !isActionAvailable(slot.opt_id)) {
            continue;
        }
        return slot.opt_id;
    }
    return ACTION_PICKER_ID_UNBOUND;
}

std::string RguiControls::getBindingLabel(int value) const {
    switch (value) {
        case SDL_CONTROLLER_BUTTON_INVALID:
            return "Unbound";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return "Up";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return "Down";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            return "Left";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            return "Right";
        case SDL_CONTROLLER_BUTTON_A:
            return "Cross";
        case SDL_CONTROLLER_BUTTON_B:
            return "Circle";
        case SDL_CONTROLLER_BUTTON_X:
            return "Square";
        case SDL_CONTROLLER_BUTTON_Y:
            return "Triangle";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return "L1";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return "R1";
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return "L3";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            return "R3";
        case SDL_CONTROLLER_BUTTON_BACK:
            return "Select";
        case SDL_CONTROLLER_BUTTON_START:
            return "Start";
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return "PS";
        default:
            break;
    }

    if (value == SDL_CONTROLLER_AXIS_TRIGGERLEFT + 100) {
        return "L2";
    }
    if (value == SDL_CONTROLLER_AXIS_TRIGGERRIGHT + 100) {
        return "R2";
    }

    char label[32];
    if (isTriggerAxis(value)) {
        snprintf(label, sizeof(label), "Axis %d", value - 100);
    } else {
        snprintf(label, sizeof(label), "Button %d", value);
    }
    return label;
}

std::string RguiControls::getOptionValue(int opt_id) const {
    auto *opt = m_ui->getConfig()->get(opt_id, m_in_game);
    if (!opt) {
        return "N/A";
    }

    if (opt_id == PEMUConfig::OptId::JOY_DEADZONE) {
        return opt->getString();
    }

    return getBindingLabel(opt->getInteger());
}

int RguiControls::handleInput(Input *input) {
    if (m_picker_visible) {
        const auto picker_action = m_action_picker->handleInput(input);
        if (picker_action == RguiMenu::CANCEL) {
            m_picker_visible = false;
            m_ui->getInput()->clear();
            return -1;
        }

        if (picker_action == RguiMenu::CONFIRM) {
            applyPhysicalBinding(m_picker_physical_code, m_action_picker->getSelectedId());
            m_picker_visible = false;

            const int sel = m_menu->getSelectedIndex();
            refresh(m_in_game);
            m_menu->setSelectedIndex(sel);
            m_ui->getInput()->clear();
        }
        return -1;
    }

    unsigned int buttons = input->getButtons();
    if ((buttons & Input::Button::Y) && m_menu->getSelectedId() != ROW_ID_DEADZONE) {
        applyPhysicalBinding(m_menu->getSelectedId(), ACTION_PICKER_ID_UNBOUND);
        refresh(m_in_game);
        m_ui->getInput()->clear();
        return -1;
    }

    const auto action = m_menu->handleInput(input);
    if (action == RguiMenu::CANCEL) {
        return 1;
    }

    if (action != RguiMenu::LEFT && action != RguiMenu::RIGHT && action != RguiMenu::CONFIRM) {
        return -1;
    }

    const int sel = m_menu->getSelectedIndex();
    const int row_id = m_menu->getSelectedId();

    if (row_id == ROW_ID_DEADZONE) {
        const int direction = action == RguiMenu::LEFT ? -1 : 1;
        if (action == RguiMenu::LEFT || action == RguiMenu::RIGHT) {
            cycleDeadZone(direction);
        }
    } else if (action == RguiMenu::CONFIRM) {
        openActionPicker(row_id);
    }

    refresh(m_in_game);
    m_menu->setSelectedIndex(sel);
    return -1;
}

void RguiControls::draw(Transform &t) {
    if (m_picker_visible) {
        m_action_picker->onDraw(t, true);
    } else {
        m_menu->onDraw(t, true);
    }
}
