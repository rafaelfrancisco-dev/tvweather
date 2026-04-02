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

    void refreshLanguage(tvweather::Language language);
    void rebuildMenuBar();
    void rebuildStatusLine();
    void refreshOpenWindows();
    void showOptions();
    void showLocationPicker();
    void showForecast(int globalIdLocal, const std::string& localName);
    void showWarnings();

    std::vector<IpmaLocation>    locations_;
    std::vector<IpmaWeatherType> weatherTypes_;
    tvweather::Language          language_ {tvweather::Language::English};
};
