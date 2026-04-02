#include "tvweather/TvWeatherApp.h"
#include "tvweather/Commands.h"
#include "tvweather/LocationPickerDialog.h"
#include "tvweather/ForecastWindow.h"
#include "tvweather/WarningsWindow.h"

TvWeatherApp::TvWeatherApp() :
    TProgInit(&TvWeatherApp::initStatusLine,
              &TvWeatherApp::initMenuBar,
              &TvWeatherApp::initDeskTop)
{
    locations_    = IpmaClient::fetchLocations();
    weatherTypes_ = IpmaClient::fetchWeatherTypes();
    showLocationPicker();
}

void TvWeatherApp::handleEvent(TEvent& event)
{
    TApplication::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
        case cmOpenLocationPicker:
            showLocationPicker();
            clearEvent(event);
            break;
        case cmShowWarnings:
            showWarnings();
            clearEvent(event);
            break;
        default:
            break;
        }
    }
}

TMenuBar* TvWeatherApp::initMenuBar(TRect bounds)
{
    bounds.b.y = bounds.a.y + 1;

    return new TMenuBar(bounds,
        *new TSubMenu("~W~eather", kbAltW) +
            *new TMenuItem("~P~ick Location...", cmOpenLocationPicker, kbF3, hcNoContext, "F3") +
            *new TMenuItem("Active ~W~arnings",  cmShowWarnings,       kbF4, hcNoContext, "F4") +
            newLine() +
            *new TMenuItem("E~x~it", cmQuit, kbCtrlQ, hcNoContext, "Ctrl-Q"));
}

TStatusLine* TvWeatherApp::initStatusLine(TRect bounds)
{
    bounds.a.y = bounds.b.y - 1;

    return new TStatusLine(bounds,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Ctrl-Q~ Exit", kbCtrlQ, cmQuit) +
            *new TStatusItem(0, kbF10, cmMenu));
}

void TvWeatherApp::showLocationPicker()
{
    if (locations_.empty())
        return;

    auto* dlg = new LocationPickerDialog(locations_);
    const ushort result = TProgram::deskTop->execView(dlg);
    if (result == cmOK) {
        const auto loc = dlg->selectedLocation();
        delete dlg;
        showForecast(loc.globalIdLocal, loc.local);
    } else {
        delete dlg;
    }
}

void TvWeatherApp::showForecast(int globalIdLocal, const std::string& localName)
{
    const auto forecast = IpmaClient::fetchForecast(globalIdLocal, localName);
    insertWindow(new ForecastWindow(forecast, weatherTypes_));
}

void TvWeatherApp::showWarnings()
{
    const auto warnings = IpmaClient::fetchWarnings();
    insertWindow(new WarningsWindow(warnings));
}
