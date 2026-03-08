//
// RGUI file browser for ROM selection
//

#ifndef FBNEO_VITA_RGUI_FILEBROWSER_H
#define FBNEO_VITA_RGUI_FILEBROWSER_H

#include "rgui_menu.h"
#include "cross2d/c2d.h"
#include <string>
#include <vector>
#include <functional>

class RguiFileBrowser {
public:
    RguiFileBrowser(c2d::Renderer *renderer, c2d::Font *font, const std::string &startPath,
                    const std::vector<std::string> &extensions);
    ~RguiFileBrowser();

    // returns: 0=file selected, 1=cancelled, -1=navigating
    int handleInput(c2d::Input *input);

    void draw(c2d::Transform &t);

    std::string getSelectedPath() const;
    std::string getCurrentPath() const;

    void setPath(const std::string &path);

private:
    void refreshList();

    c2d::Renderer *m_renderer;
    RguiMenu *m_menu;
    std::string m_current_path;
    std::vector<std::string> m_extensions;
    std::vector<c2d::Io::File> m_entries;
    std::string m_selected_file;
};

#endif // FBNEO_VITA_RGUI_FILEBROWSER_H
