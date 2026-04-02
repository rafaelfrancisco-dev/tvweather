#include "tvweather/TvWeatherApp.h"

namespace
{
constexpr int cmOpenStarterWindow = 100;

class StarterWindow : public TWindow
{
public:
    StarterWindow() :
        TWindowInit(TWindow::initFrame),
        TWindow(TRect(10, 3, 70, 18), "tvweather", 0)
    {
        flags &= ~(wfZoom | wfGrow);
        options |= ofCentered;

        insert(new TStaticText(
            TRect(2, 2, 57, 12),
            "tvweather\n"
            "\n"
            "Turbo Vision scaffold is running.\n"
            "\n"
            "Next steps:\n"
            "  * add forecast views\n"
            "  * wire weather data sources\n"
            "  * expand keyboard-driven workflows\n"
            "\n"
            "Press Ctrl-Q to quit or F2 to reopen this window."
        ));
    }
};
} // namespace

TvWeatherApp::TvWeatherApp() :
    TProgInit(&TvWeatherApp::initStatusLine,
              &TvWeatherApp::initMenuBar,
              &TvWeatherApp::initDeskTop)
{
    showStarterWindow();
}

void TvWeatherApp::handleEvent(TEvent &event)
{
    TApplication::handleEvent(event);

    if (event.what == evCommand)
    {
        switch (event.message.command)
        {
        case cmOpenStarterWindow:
            showStarterWindow();
            clearEvent(event);
            break;
        default:
            break;
        }
    }
}

TMenuBar *TvWeatherApp::initMenuBar(TRect bounds)
{
    bounds.b.y = bounds.a.y + 1;

    return new TMenuBar(bounds,
        *new TSubMenu("~A~pp", kbAltA) +
            *new TMenuItem("~O~pen starter", cmOpenStarterWindow, kbF2, hcNoContext, "F2") +
            newLine() +
            *new TMenuItem("E~x~it", cmQuit, kbCtrlQ, hcNoContext, "Ctrl-Q"));
}

TStatusLine *TvWeatherApp::initStatusLine(TRect bounds)
{
    bounds.a.y = bounds.b.y - 1;

    return new TStatusLine(bounds,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~F2~ Starter", kbF2, cmOpenStarterWindow) +
            *new TStatusItem("~Ctrl-Q~ Exit", kbCtrlQ, cmQuit) +
            *new TStatusItem(0, kbF10, cmMenu));
}

void TvWeatherApp::showStarterWindow()
{
    insertWindow(new StarterWindow);
}
