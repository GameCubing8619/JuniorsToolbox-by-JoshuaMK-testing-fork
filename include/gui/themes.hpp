#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

#include <imgui.h>

#include "fsystem.hpp"

namespace Toolbox::UI {

    class ITheme {
    public:
        virtual ~ITheme() = default;

        virtual std::string_view name() const = 0;
        virtual bool apply()                  = 0;
    };

    class ConfigTheme : public ITheme {
    public:
        ConfigTheme(std::string_view name);
        ConfigTheme(std::string_view name, const ImGuiStyle &theme);
        ~ConfigTheme() override = default;

        std::string_view name() const override { return m_name; }
        bool apply() override;

        void saveToFile(const std::filesystem::path &path);
    protected:
        void loadFromFile(const std::filesystem::path &path);

    private:
        bool m_load_ok = false;
        std::string m_name;
        ImGuiStyle m_style;
    };

    class ThemeManager {
    public:
        ThemeManager() = default;
        ~ThemeManager() = default;

        static ThemeManager &instance() {
            static ThemeManager instance_;
            return instance_;
        }

        void addTheme(std::shared_ptr<ITheme> theme) { m_themes.push_back(theme); }
        std::vector<std::shared_ptr<ITheme>> themes() const { return m_themes; }

        std::expected<void, FSError> initialize();

    private:
        std::vector<std::shared_ptr<ITheme>> m_themes;
    };

}  // namespace Toolbox::UI