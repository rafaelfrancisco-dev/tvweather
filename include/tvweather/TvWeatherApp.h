#pragma once

// Turbo Vision exposes declarations through opt-in Uses_* macros.
#define Uses_TApplication
#define Uses_TDeskTop
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TMenuBar
#define Uses_TMenuItem
#define Uses_TRect
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TStatusLine
#define Uses_TSubMenu
#define Uses_TWindow
#include <tvision/tv.h>

#include "ipma/Client.h"
#include "tvweather/Language.h"
#include "tvweather/Settings.h"
#include <optional>
#include <string>
#include <vector>

class TvWeatherApp : public TApplication
{
public:
    TvWeatherApp();

    void handleEvent(TEvent& event) override;

private:
    static TMenuBar*    initMenuBar(TRect bounds);
    static TStatusLine* initStatusLine(TRect bounds);

    void applySavedPalette(const std::optional<tvweather::SavedPalette>& palette);
    void clearSelectedLocation();
    void closeLocationDependentWindows();
    bool ensureSelectedLocation();
    std::optional<tvweather::SavedLocation> pickLocation(const std::vector<IpmaLocation>& locations);
    void refreshLanguage(tvweather::Language language);
    void rebuildMenuBar();
    void rebuildStatusLine();
    void refreshOpenWindows();
    void saveSettings() const;
    void selectLocation(const tvweather::SavedLocation& location);
    void showColors();
    void showOptions();
    void showForecast();
    void showLocationPicker();
    void showForecast(int globalIdLocal, const std::string& localName);
    void showWarnings();
    void showUv();
    void showSea();
    void updateLocationCommandState();

    std::vector<IpmaLocation>    locations_;
    std::vector<IpmaWeatherType> weatherTypes_;
    std::vector<IpmaWindType>    windTypes_;
    std::vector<IpmaWindDirectionType> windDirectionTypes_;
    tvweather::Language          language_ {tvweather::Language::English};
    tvweather::SavedPalette      originalPalette_;
    std::optional<tvweather::SavedPalette>  palette_;
    std::optional<tvweather::SavedLocation> selectedLocation_;
};
