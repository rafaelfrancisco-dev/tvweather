#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "MiniTest.h"

#include "tvweather/Localization.h"
#include "tvweather/Settings.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("weatherDescriptionForType prefers English descriptions in English mode")
{
    const std::vector<IpmaWeatherType> types {
        {1, "Ceu limpo", "Clear sky"},
        {2, "Poucas nuvens", "Partly cloudy"},
    };

    CHECK(tvweather::weatherDescriptionForType(types, 2, tvweather::Language::English)
          == "Partly cloudy");
}

TEST_CASE("weatherDescriptionForType prefers Portuguese descriptions in Portuguese mode")
{
    const std::vector<IpmaWeatherType> types {
        {1, "Ceu limpo", "Clear sky"},
        {2, "Poucas nuvens", "Partly cloudy"},
    };

    CHECK(tvweather::weatherDescriptionForType(types, 2, tvweather::Language::Portuguese)
          == "Poucas nuvens");
}

TEST_CASE("uiStrings returns localized chrome labels")
{
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).optionsMenu)
          == "~O~ptions");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).clearLocation)
          == "~C~lear Location");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).weatherMenu)
          == "~M~eteorologia");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).optionsMenu)
          == "~O~pções");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).optionsDialogTitle)
          == "Opções");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).colors)
          == "~C~olors...");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).statusNoLocation)
          == "Localidade: nenhuma seleccionada");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).forecastDetails)
          == "Details");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).seaClosestNotice)
          == "Sea conditions are shown for the closest coastal location.");
}

TEST_CASE("rainRiskLabel maps precipitation percentages into localized severities")
{
    CHECK(std::string(tvweather::rainRiskLabel(10, tvweather::Language::English)) == "Low");
    CHECK(std::string(tvweather::rainRiskLabel(35, tvweather::Language::English))
          == "Moderate");
    CHECK(std::string(tvweather::rainRiskLabel(80, tvweather::Language::Portuguese))
          == "Alto");
}

TEST_CASE("windStrengthLabel maps wind classes into localized severities")
{
    CHECK(std::string(tvweather::windStrengthLabel(1, tvweather::Language::English))
          == "Light");
    CHECK(std::string(tvweather::windStrengthLabel(2, tvweather::Language::Portuguese))
          == "Moderado");
    CHECK(std::string(tvweather::windStrengthLabel(5, tvweather::Language::English))
          == "Strong");
}

TEST_CASE("loadLanguageFromFile defaults to English when config is missing")
{
    CHECK(tvweather::loadLanguageFromFile(fs::path("does-not-exist.json"))
          == tvweather::Language::English);
}

TEST_CASE("loadLanguageFromFile reads en and pt values")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-tests";
    fs::create_directories(tempDir);

    const fs::path englishPath = tempDir / "english.json";
    {
        std::ofstream output(englishPath);
        output << "{\n  \"language\": \"en\"\n}\n";
    }
    CHECK(tvweather::loadLanguageFromFile(englishPath) == tvweather::Language::English);

    const fs::path portuguesePath = tempDir / "portuguese.json";
    {
        std::ofstream output(portuguesePath);
        output << "{\n  \"language\": \"pt\"\n}\n";
    }
    CHECK(tvweather::loadLanguageFromFile(portuguesePath) == tvweather::Language::Portuguese);

    fs::remove_all(tempDir);
}

TEST_CASE("loadSettingsFromFile reads language and selected location")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-settings-load";
    fs::create_directories(tempDir);

    const fs::path configPath = tempDir / "config.json";
    {
        std::ofstream output(configPath);
        output << "{\n"
                  "  \"language\": \"pt\",\n"
                  "  \"selectedLocation\": {\n"
                  "    \"globalIdLocal\": 1110600,\n"
                  "    \"local\": \"Lisboa\"\n"
                  "  }\n"
                  "}\n";
    }

    const auto settings = tvweather::loadSettingsFromFile(configPath);
    CHECK(settings.language == tvweather::Language::Portuguese);
    REQUIRE(settings.selectedLocation.has_value());
    CHECK(settings.selectedLocation->globalIdLocal == 1110600);
    CHECK(settings.selectedLocation->local == "Lisboa");

    fs::remove_all(tempDir);
}

TEST_CASE("loadSettingsFromFile reads persisted palette data")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-settings-palette";
    fs::create_directories(tempDir);

    const fs::path configPath = tempDir / "config.json";
    {
        std::ofstream output(configPath);
        output << "{\n"
                  "  \"language\": \"en\",\n"
                  "  \"palette\": {\n"
                  "    \"mode\": 1,\n"
                  "    \"entries\": [112, 113, 114]\n"
                  "  }\n"
                  "}\n";
    }

    const auto settings = tvweather::loadSettingsFromFile(configPath);
    REQUIRE(settings.palette.has_value());
    CHECK(settings.palette->mode == 1);
    CHECK(settings.palette->entries == std::vector<int>({112, 113, 114}));

    fs::remove_all(tempDir);
}

TEST_CASE("loadSettingsFromFile ignores invalid selected location values")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-settings-invalid";
    fs::create_directories(tempDir);

    const fs::path configPath = tempDir / "config.json";
    {
        std::ofstream output(configPath);
        output << "{\n"
                  "  \"language\": \"en\",\n"
                  "  \"selectedLocation\": {\n"
                  "    \"globalIdLocal\": 0,\n"
                  "    \"local\": \"\"\n"
                  "  }\n"
                  "}\n";
    }

    const auto settings = tvweather::loadSettingsFromFile(configPath);
    CHECK(settings.language == tvweather::Language::English);
    CHECK(!settings.selectedLocation.has_value());

    fs::remove_all(tempDir);
}

TEST_CASE("loadSettingsFromFile ignores invalid palette values")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-settings-bad-palette";
    fs::create_directories(tempDir);

    const fs::path configPath = tempDir / "config.json";
    {
        std::ofstream output(configPath);
        output << "{\n"
                  "  \"palette\": {\n"
                  "    \"mode\": 0,\n"
                  "    \"entries\": [10, -1, 300]\n"
                  "  }\n"
                  "}\n";
    }

    const auto settings = tvweather::loadSettingsFromFile(configPath);
    CHECK(!settings.palette.has_value());

    fs::remove_all(tempDir);
}

TEST_CASE("loadLanguageFromFile falls back to English on invalid content")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-invalid";
    fs::create_directories(tempDir);

    const fs::path invalidPath = tempDir / "invalid.json";
    {
        std::ofstream output(invalidPath);
        output << "{not valid json}\n";
    }

    CHECK(tvweather::loadLanguageFromFile(invalidPath) == tvweather::Language::English);
    fs::remove_all(tempDir);
}

TEST_CASE("saveLanguageToFile writes the selected language")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-save";
    const fs::path configPath = tempDir / "config.json";

    REQUIRE(tvweather::saveLanguageToFile(configPath, tvweather::Language::Portuguese));
    CHECK(tvweather::loadLanguageFromFile(configPath) == tvweather::Language::Portuguese);

    fs::remove_all(tempDir);
}

TEST_CASE("saveSettingsToFile writes language and selected location")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-save-settings";
    const fs::path configPath = tempDir / "config.json";

    tvweather::AppSettings settings;
    settings.language = tvweather::Language::Portuguese;
    settings.selectedLocation = tvweather::SavedLocation {1110600, "Lisboa"};

    REQUIRE(tvweather::saveSettingsToFile(configPath, settings));

    const auto loaded = tvweather::loadSettingsFromFile(configPath);
    CHECK(loaded.language == tvweather::Language::Portuguese);
    REQUIRE(loaded.selectedLocation.has_value());
    CHECK(loaded.selectedLocation->globalIdLocal == 1110600);
    CHECK(loaded.selectedLocation->local == "Lisboa");

    fs::remove_all(tempDir);
}

TEST_CASE("saveSettingsToFile round-trips palette data")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-save-palette";
    const fs::path configPath = tempDir / "config.json";

    tvweather::AppSettings settings;
    settings.language = tvweather::Language::Portuguese;
    settings.selectedLocation = tvweather::SavedLocation {1110600, "Lisboa"};
    settings.palette = tvweather::SavedPalette {0, {10, 20, 30, 40}};

    REQUIRE(tvweather::saveSettingsToFile(configPath, settings));

    const auto loaded = tvweather::loadSettingsFromFile(configPath);
    CHECK(loaded.language == tvweather::Language::Portuguese);
    REQUIRE(loaded.selectedLocation.has_value());
    CHECK(loaded.selectedLocation->local == "Lisboa");
    REQUIRE(loaded.palette.has_value());
    CHECK(loaded.palette->mode == 0);
    CHECK(loaded.palette->entries == std::vector<int>({10, 20, 30, 40}));

    fs::remove_all(tempDir);
}

TEST_CASE("saveLanguageToFile preserves the selected location")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-preserve-location";
    const fs::path configPath = tempDir / "config.json";

    tvweather::AppSettings settings;
    settings.language = tvweather::Language::English;
    settings.selectedLocation = tvweather::SavedLocation {1010500, "Porto"};
    REQUIRE(tvweather::saveSettingsToFile(configPath, settings));

    REQUIRE(tvweather::saveLanguageToFile(configPath, tvweather::Language::Portuguese));

    const auto loaded = tvweather::loadSettingsFromFile(configPath);
    CHECK(loaded.language == tvweather::Language::Portuguese);
    REQUIRE(loaded.selectedLocation.has_value());
    CHECK(loaded.selectedLocation->globalIdLocal == 1010500);
    CHECK(loaded.selectedLocation->local == "Porto");

    fs::remove_all(tempDir);
}

TEST_CASE("saveSettingsToFile can clear the selected location while keeping language")
{
    const fs::path tempDir = fs::temp_directory_path() / "tvweather-support-clear-location";
    const fs::path configPath = tempDir / "config.json";

    tvweather::AppSettings settings;
    settings.language = tvweather::Language::Portuguese;
    settings.selectedLocation = tvweather::SavedLocation {1110600, "Lisboa"};
    REQUIRE(tvweather::saveSettingsToFile(configPath, settings));

    settings.selectedLocation.reset();
    REQUIRE(tvweather::saveSettingsToFile(configPath, settings));

    const auto loaded = tvweather::loadSettingsFromFile(configPath);
    CHECK(loaded.language == tvweather::Language::Portuguese);
    CHECK(!loaded.selectedLocation.has_value());

    fs::remove_all(tempDir);
}
