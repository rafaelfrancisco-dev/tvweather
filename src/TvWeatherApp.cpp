#include "tvweather/TvWeatherApp.h"
#include "tvweather/Commands.h"
#include "tvweather/LocationPickerDialog.h"
#include "tvweather/OptionsDialog.h"
#include "tvweather/ForecastWindow.h"
#include "tvweather/Localization.h"
#include "tvweather/Settings.h"
#include "tvweather/WarningsWindow.h"

namespace {

TMenuBar* buildMenuBar(TRect bounds, tvweather::Language language)
{
    const auto& strings = tvweather::uiStrings(language);
    const TKey weatherMenuKey = language == tvweather::Language::Portuguese
        ? kbAltM
        : kbAltW;
    bounds.b.y = bounds.a.y + 1;

    return new TMenuBar(bounds,
        *new TSubMenu(strings.weatherMenu, weatherMenuKey) +
            *new TMenuItem(strings.pickLocation, cmOpenLocationPicker, kbF3, hcNoContext, "F3") +
            *new TMenuItem(strings.activeWarnings, cmShowWarnings, kbF4, hcNoContext, "F4") +
            newLine() +
            *new TMenuItem(strings.exitApp, cmQuit, kbCtrlQ, hcNoContext, "Ctrl-Q") +
        *new TSubMenu(strings.optionsMenu, kbAltO) +
            *new TMenuItem(strings.preferences, cmOpenOptions, kbNoKey, hcNoContext));
}

TStatusLine* buildStatusLine(TRect bounds, tvweather::Language language)
{
    const auto& strings = tvweather::uiStrings(language);
    bounds.a.y = bounds.b.y - 1;

    return new TStatusLine(bounds,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem(strings.statusExit, kbCtrlQ, cmQuit) +
            *new TStatusItem(0, kbF10, cmMenu));
}

void refreshWindowLanguage(TView* view, void* languagePtr)
{
    const auto language = *static_cast<tvweather::Language*>(languagePtr);
    if (auto* forecastWindow = dynamic_cast<ForecastWindow*>(view))
        forecastWindow->setLanguage(language);
    else if (auto* warningsWindow = dynamic_cast<WarningsWindow*>(view))
        warningsWindow->setLanguage(language);
}

} // namespace

TvWeatherApp::TvWeatherApp() :
    TProgInit(&TvWeatherApp::initStatusLine,
              &TvWeatherApp::initMenuBar,
              &TvWeatherApp::initDeskTop)
{
    language_ = tvweather::loadLanguage();
    rebuildStatusLine();
    rebuildMenuBar();
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
        case cmOpenOptions:
            showOptions();
            clearEvent(event);
            break;
        default:
            break;
        }
    }
}

TMenuBar* TvWeatherApp::initMenuBar(TRect bounds)
{
    return buildMenuBar(bounds, tvweather::Language::English);
}

TStatusLine* TvWeatherApp::initStatusLine(TRect bounds)
{
    return buildStatusLine(bounds, tvweather::Language::English);
}

void TvWeatherApp::refreshLanguage(tvweather::Language language)
{
    if (language_ == language)
        return;

    language_ = language;
    tvweather::saveLanguage(language_);
    rebuildStatusLine();
    rebuildMenuBar();
    refreshOpenWindows();
    TProgram::deskTop->drawView();
}

void TvWeatherApp::rebuildMenuBar()
{
    if (TProgram::menuBar != nullptr) {
        remove(TProgram::menuBar);
        destroy(TProgram::menuBar);
        TProgram::menuBar = nullptr;
    }

    TProgram::menuBar = buildMenuBar(getExtent(), language_);
    insert(TProgram::menuBar);
}

void TvWeatherApp::rebuildStatusLine()
{
    if (TProgram::statusLine != nullptr) {
        remove(TProgram::statusLine);
        destroy(TProgram::statusLine);
        TProgram::statusLine = nullptr;
    }

    TProgram::statusLine = buildStatusLine(getExtent(), language_);
    insert(TProgram::statusLine);
}

void TvWeatherApp::showOptions()
{
    auto* dialog = new OptionsDialog(language_);
    const ushort result = TProgram::deskTop->execView(dialog);
    if (result == cmOK) {
        const auto selectedLanguage = dialog->selectedLanguage();
        delete dialog;
        refreshLanguage(selectedLanguage);
    } else {
        delete dialog;
    }
}

void TvWeatherApp::refreshOpenWindows()
{
    TProgram::deskTop->forEach(refreshWindowLanguage, &language_);
}

void TvWeatherApp::showLocationPicker()
{
    if (locations_.empty())
        return;

    auto* dlg = new LocationPickerDialog(locations_, language_);
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
    insertWindow(new ForecastWindow(forecast, weatherTypes_, language_));
}

void TvWeatherApp::showWarnings()
{
    const auto warnings = IpmaClient::fetchWarnings();
    insertWindow(new WarningsWindow(warnings, language_));
}
