//
// RGUI controls menu
//

#ifndef FBNEO_VITA_RGUI_CONTROLS_H
#define FBNEO_VITA_RGUI_CONTROLS_H

#include "rgui_menu.h"
#include "cross2d/c2d.h"

namespace pemu {
    class UiMain;
}

class RguiControls {
public:
    RguiControls(pemu::UiMain *ui, c2d::Renderer *renderer, c2d::Font *font);
    ~RguiControls();

    void refresh(bool in_game);

    // returns: 1=cancelled, -1=navigating
    int handleInput(c2d::Input *input);

    void draw(c2d::Transform &t);

private:
    void cycleDeadZone(int direction);
    void openActionPicker(int physical_code);
    void buildActionPicker(int physical_code);
    void applyPhysicalBinding(int physical_code, int action_opt_id);
    void saveCurrentScope() const;
    std::string getRuntimeActionLabel(int opt_id) const;
    bool isActionAvailable(int opt_id) const;
    std::string getActionLabel(int opt_id) const;
    std::string getBoundActionLabel(int physical_code) const;
    int getPrimaryBoundActionOptId(int physical_code, bool available_only = false) const;
    std::string getBindingLabel(int value) const;
    std::string getOptionValue(int opt_id) const;

    pemu::UiMain *m_ui;
    c2d::Renderer *m_renderer;
    RguiMenu *m_menu;
    RguiMenu *m_action_picker;
    bool m_in_game = false;
    bool m_picker_visible = false;
    int m_picker_physical_code = -1;
};

#endif // FBNEO_VITA_RGUI_CONTROLS_H
