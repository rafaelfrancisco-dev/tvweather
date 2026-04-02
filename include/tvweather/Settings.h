#pragma once

#include "tvweather/Language.h"
#include <filesystem>

namespace tvweather {

std::filesystem::path defaultSettingsPath();
Language loadLanguage();
Language loadLanguageFromFile(const std::filesystem::path& path);
bool saveLanguage(Language language);
bool saveLanguageToFile(const std::filesystem::path& path, Language language);

} // namespace tvweather
