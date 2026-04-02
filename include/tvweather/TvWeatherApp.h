#pragma once

// Turbo Vision exposes declarations through opt-in Uses_* macros.
#define Uses_TApplication
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TMenuBar
#define Uses_TMenuItem
#define Uses_TRect
#define Uses_TStaticText
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TStatusLine
#define Uses_TSubMenu
#define Uses_TWindow
#include <tvision/tv.h>

class TvWeatherApp : public TApplication
{
public:
    TvWeatherApp();

    void handleEvent(TEvent &event) override;

private:
    static TMenuBar *initMenuBar(TRect bounds);
    static TStatusLine *initStatusLine(TRect bounds);

    void showStarterWindow();
};
