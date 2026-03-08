//
// RGUI file browser for ROM selection
//

#include "rgui_filebrowser.h"
#include <algorithm>

using namespace c2d;

namespace {
    std::string getParentPath(const std::string &currentPath) {
        std::string path = currentPath;
        if (path.length() > 1 && path.back() == '/') {
            path.pop_back();
        }

        auto colon = path.rfind(':');
        if (colon != std::string::npos) {
            const size_t rootLength = colon + 2; // "ux0:/"
            if (path.length() <= rootLength) {
                return path.substr(0, rootLength);
            }

            auto pos = path.rfind('/');
            if (pos != std::string::npos) {
                if (pos == colon + 1) {
                    return path.substr(0, rootLength);
                }
                return path.substr(0, pos + 1);
            }
        }

        auto pos = path.rfind('/');
        if (pos != std::string::npos) {
            return path.substr(0, pos + 1);
        }

        return currentPath;
    }
}

RguiFileBrowser::RguiFileBrowser(Renderer *renderer, Font *font, const std::string &startPath,
                                   const std::vector<std::string> &extensions)
    : m_renderer(renderer), m_current_path(startPath), m_extensions(extensions) {
    m_menu = new RguiMenu(renderer, font, "Browse: " + startPath, {});
    refreshList();
}

RguiFileBrowser::~RguiFileBrowser() {
    delete m_menu;
}

void RguiFileBrowser::refreshList() {
    m_entries.clear();
    std::vector<RguiMenuItem> items;

    auto dirList = m_renderer->getIo()->getDirList(m_current_path, true);

    // add parent directory entry
    {
        RguiMenuItem item;
        item.label = "..";
        item.id = -1;
        item.is_submenu = true;
        items.push_back(item);
        Io::File f("..", m_current_path + "..", Io::Type::Directory);
        m_entries.push_back(f);
    }

    // sort: directories first, then files
    std::sort(dirList.begin(), dirList.end(), Io::compare);

    int id = 0;
    for (const auto &entry : dirList) {
        if (entry.name == "." || entry.name == "..") continue;

        if (entry.type == Io::Type::Directory) {
            RguiMenuItem item;
            item.label = "[" + entry.name + "]";
            item.id = id++;
            item.is_submenu = true;
            items.push_back(item);
            m_entries.push_back(entry);
        } else {
            // check extension filter
            bool match = m_extensions.empty();
            if (!match) {
                std::string name_lower = Utility::toLower(entry.name);
                for (const auto &ext : m_extensions) {
                    std::string ext_lower = Utility::toLower(ext);
                    if (name_lower.length() >= ext_lower.length() &&
                        name_lower.compare(name_lower.length() - ext_lower.length(),
                                           ext_lower.length(), ext_lower) == 0) {
                        match = true;
                        break;
                    }
                }
            }
            if (match) {
                RguiMenuItem item;
                item.label = entry.name;
                item.id = id++;
                item.is_submenu = false;
                items.push_back(item);
                m_entries.push_back(entry);
            }
        }
    }

    m_menu->setItems(items);
    m_menu->setTitle("Browse: " + m_current_path);
}

int RguiFileBrowser::handleInput(Input *input) {
    auto action = m_menu->handleInput(input);

    if (action == RguiMenu::CONFIRM) {
        int idx = m_menu->getSelectedIndex();
        if (idx >= 0 && idx < (int)m_entries.size()) {
            const auto &entry = m_entries[idx];
            if (entry.type == Io::Type::Directory) {
                if (entry.name == "..") {
                    m_current_path = getParentPath(m_current_path);
                } else {
                    m_current_path = entry.path + "/";
                }
                refreshList();
                return -1; // still navigating
            } else {
                m_selected_file = entry.path;
                return 0; // file selected
            }
        }
    } else if (action == RguiMenu::CANCEL) {
        return 1; // cancelled
    }

    return -1; // navigating
}

void RguiFileBrowser::draw(Transform &t) {
    m_menu->onDraw(t, true);
}

std::string RguiFileBrowser::getSelectedPath() const {
    return m_selected_file;
}

std::string RguiFileBrowser::getCurrentPath() const {
    return m_current_path;
}

void RguiFileBrowser::setPath(const std::string &path) {
    m_current_path = path;
    refreshList();
}
