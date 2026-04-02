#pragma once

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>
#include <vector>

namespace tvweather {

struct UiStrings {
    const char* weatherMenu;
    const char* pickLocation;
    const char* activeWarnings;
    const char* exitApp;
    const char* optionsMenu;
    const char* preferences;
    const char* statusExit;
    const char* optionsDialogTitle;
    const char* languageLabel;
    const char* languageEnglish;
    const char* languagePortuguese;
    const char* ok;
    const char* locationDialogTitle;
    const char* openForecast;
    const char* cancel;
    const char* forecastDate;
    const char* forecastMin;
    const char* forecastMax;
    const char* forecastRain;
    const char* forecastWind;
    const char* forecastConditions;
    const char* forecastOverview;
    const char* forecastDetails;
    const char* forecastTemperature;
    const char* forecastRainRisk;
    const char* forecastWindStrength;
    const char* forecastNavigation;
    const char* forecastEmpty;
    const char* warningsTitle;
    const char* warningsEmpty;
    const char* warningsFrom;
    const char* warningsTo;
};

const UiStrings& uiStrings(Language language) noexcept;
std::string forecastWindowTitle(const std::string& localName, Language language);
std::string weatherDescriptionForType(const std::vector<IpmaWeatherType>& types,
                                      int id,
                                      Language language);
const char* rainRiskLabel(int precipitaProb, Language language) noexcept;
const char* windStrengthLabel(int classWindSpeed, Language language) noexcept;
const char* languageCode(Language language) noexcept;
Language languageFromCodeOrDefault(const std::string& code) noexcept;

} // namespace tvweather
