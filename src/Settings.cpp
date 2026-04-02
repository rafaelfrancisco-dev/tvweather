#include "tvweather/Settings.h"
#include "tvweather/Localization.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace tvweather {

namespace fs = std::filesystem;

namespace {

fs::path settingsDirectory()
{
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA"))
        return fs::path(appData) / "tvweather";
#else
    if (const char* home = std::getenv("HOME"))
        return fs::path(home) / ".config" / "tvweather";
#endif
    return {};
}

} // namespace

fs::path defaultSettingsPath()
{
    const fs::path dir = settingsDirectory();
    if (dir.empty())
        return {};
    return dir / "config.json";
}

Language loadLanguage()
{
    const fs::path path = defaultSettingsPath();
    if (path.empty())
        return Language::English;
    return loadLanguageFromFile(path);
}

Language loadLanguageFromFile(const fs::path& path)
{
    if (path.empty())
        return Language::English;

    std::ifstream input(path);
    if (!input)
        return Language::English;

    try {
        const auto json = nlohmann::json::parse(input);
        if (json.contains("language") && json["language"].is_string())
            return languageFromCodeOrDefault(json["language"].get<std::string>());
    } catch (...) {
    }

    return Language::English;
}

bool saveLanguage(Language language)
{
    const fs::path path = defaultSettingsPath();
    if (path.empty())
        return false;
    return saveLanguageToFile(path, language);
}

bool saveLanguageToFile(const fs::path& path, Language language)
{
    if (path.empty())
        return false;

    try {
        if (!path.parent_path().empty())
            fs::create_directories(path.parent_path());

        std::ofstream output(path);
        if (!output)
            return false;

        nlohmann::json json;
        json["language"] = languageCode(language);
        output << json.dump(2) << '\n';
        return static_cast<bool>(output);
    } catch (...) {
        return false;
    }
}

} // namespace tvweather
