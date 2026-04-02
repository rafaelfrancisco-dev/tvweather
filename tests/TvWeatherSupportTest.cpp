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
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).weatherMenu)
          == "~M~eteorologia");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).optionsMenu)
          == "~O~pções");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::Portuguese).optionsDialogTitle)
          == "Opções");
    CHECK(std::string(tvweather::uiStrings(tvweather::Language::English).forecastDetails)
          == "Details");
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
