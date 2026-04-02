#include "tvweather/Settings.h"
#include "tvweather/Localization.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace tvweather {

namespace fs = std::filesystem;

namespace {

using Json = nlohmann::json;

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

AppSettings parseSettings(const Json& json)
{
    AppSettings settings;

    if (json.contains("language") && json["language"].is_string())
        settings.language = languageFromCodeOrDefault(json["language"].get<std::string>());

    if (json.contains("selectedLocation") && json["selectedLocation"].is_object()) {
        const auto& location = json["selectedLocation"];
        if (location.contains("globalIdLocal")
            && location["globalIdLocal"].is_number_integer()
            && location.contains("local")
            && location["local"].is_string()) {
            SavedLocation savedLocation;
            savedLocation.globalIdLocal = location["globalIdLocal"].get<int>();
            savedLocation.local = location["local"].get<std::string>();

            if (savedLocation.globalIdLocal > 0 && !savedLocation.local.empty())
                settings.selectedLocation = std::move(savedLocation);
        }
    }

    return settings;
}

Json serializeSettings(const AppSettings& settings)
{
    Json json;
    json["language"] = languageCode(settings.language);

    if (settings.selectedLocation.has_value()) {
        json["selectedLocation"] = {
            {"globalIdLocal", settings.selectedLocation->globalIdLocal},
            {"local", settings.selectedLocation->local},
        };
    }

    return json;
}

} // namespace

fs::path defaultSettingsPath()
{
    const fs::path dir = settingsDirectory();
    if (dir.empty())
        return {};
    return dir / "config.json";
}

AppSettings loadSettings()
{
    const fs::path path = defaultSettingsPath();
    if (path.empty())
        return {};
    return loadSettingsFromFile(path);
}

AppSettings loadSettingsFromFile(const fs::path& path)
{
    if (path.empty())
        return {};

    std::ifstream input(path);
    if (!input)
        return {};

    try {
        return parseSettings(Json::parse(input));
    } catch (...) {
    }

    return {};
}

bool saveSettings(const AppSettings& settings)
{
    const fs::path path = defaultSettingsPath();
    if (path.empty())
        return false;
    return saveSettingsToFile(path, settings);
}

bool saveSettingsToFile(const fs::path& path, const AppSettings& settings)
{
    if (path.empty())
        return false;

    try {
        if (!path.parent_path().empty())
            fs::create_directories(path.parent_path());

        std::ofstream output(path);
        if (!output)
            return false;

        const Json json = serializeSettings(settings);
        output << json.dump(2) << '\n';
        return static_cast<bool>(output);
    } catch (...) {
        return false;
    }
}

Language loadLanguage()
{
    return loadSettings().language;
}

Language loadLanguageFromFile(const fs::path& path)
{
    return loadSettingsFromFile(path).language;
}

bool saveLanguage(Language language)
{
    AppSettings settings = loadSettings();
    settings.language = language;
    return saveSettings(settings);
}

bool saveLanguageToFile(const fs::path& path, Language language)
{
    AppSettings settings = loadSettingsFromFile(path);
    settings.language = language;
    return saveSettingsToFile(path, settings);
}

} // namespace tvweather
