#pragma once

#include "tvweather/Language.h"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tvweather {

struct SavedLocation {
    int         globalIdLocal {0};
    std::string local;
};

struct SavedPalette {
    int              mode {0};
    std::vector<int> entries;
};

struct AppSettings {
    Language                     language {Language::English};
    std::optional<SavedLocation> selectedLocation;
    std::optional<SavedPalette>  palette;
};

std::filesystem::path defaultSettingsPath();
AppSettings loadSettings();
AppSettings loadSettingsFromFile(const std::filesystem::path& path);
bool saveSettings(const AppSettings& settings);
bool saveSettingsToFile(const std::filesystem::path& path,
                        const AppSettings&          settings);
Language loadLanguage();
Language loadLanguageFromFile(const std::filesystem::path& path);
bool saveLanguage(Language language);
bool saveLanguageToFile(const std::filesystem::path& path, Language language);

} // namespace tvweather
