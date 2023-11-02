#include <fstream>

#include "fsystem.hpp"
#include "gui/themes.hpp"

namespace Toolbox::UI {
    ConfigTheme::ConfigTheme(std::string_view name) : m_name(name) {
        auto cwd_result = Toolbox::current_path();
        if (!cwd_result) {
            return;
        }
        loadFromFile(cwd_result.value() / "Themes" / name / ".theme");
    }

    ConfigTheme::ConfigTheme(std::string_view name, const ImGuiStyle &theme)
        : m_name(name), m_style(theme), m_load_ok(true) {}

    bool ConfigTheme::apply() {
        if (!m_load_ok)
            return false;
        ImGui::GetStyle() = m_style;
        return true;
    }

    void ConfigTheme::loadFromFile(const std::filesystem::path &path) {
        std::ifstream in(path, std::ios::binary | std::ios::in);

        in.read(reinterpret_cast<char *>(&m_style), sizeof(m_style));

        m_load_ok = true;
    }

    void ConfigTheme::saveToFile(const std::filesystem::path &path) {
        std::ofstream out(path, std::ios::binary | std::ios::out | std::ios::ate);

        out.write(reinterpret_cast<const char *>(&m_style), sizeof(m_style));
    }

    std::expected<void, FSError> ThemeManager::initialize() {
        auto cwd_result = Toolbox::current_path();
        if (!cwd_result) {
            return make_fs_error<void>(cwd_result.error(), "Failed to get the cwd");
        }

        auto &cwd = cwd_result.value();
        for (auto &subpath : std::filesystem::directory_iterator{cwd / "Themes"}) {
            try {
                auto theme = std::make_shared<ConfigTheme>(subpath.path().stem().string());
                m_themes.emplace_back(theme);
            } catch (std::runtime_error &e) {
                return make_fs_error<void>(std::error_code(), e.what());
            }
        }

        return {};
    }

}  // namespace Toolbox::UI