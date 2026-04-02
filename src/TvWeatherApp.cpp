#define Uses_MsgBox
#include "tvweather/TvWeatherApp.h"
#include "tvweather/Commands.h"
#include "tvweather/LocationPickerDialog.h"
#include "tvweather/OptionsDialog.h"
#include "tvweather/ForecastWindow.h"
#include "tvweather/Localization.h"
#include "tvweather/Settings.h"
#include "tvweather/WarningsWindow.h"
#include "tvweather/UvWindow.h"
#include "tvweather/SeaWindow.h"

#include <cctype>

namespace {

class TopLevelMnemonicMenuBar final : public TMenuBar
{
public:
    TopLevelMnemonicMenuBar(const TRect& bounds, TSubMenu& menu) noexcept
        : TMenuBar(bounds, menu)
    {
        options |= ofPostProcess;
    }

    void handleEvent(TEvent& event) override
    {
        if (event.what == evKeyDown &&
            owner != nullptr &&
            owner->phase == TGroup::phPostProcess) {
            const char mnemonic = matchingTopLevelMnemonic(event.keyDown.charScan.charCode);
            if (mnemonic != 0) {
                TEvent translated = event;
                translated.keyDown.keyCode = getAltCode(mnemonic);
                translated.keyDown.charScan.charCode = 0;
                TMenuBar::handleEvent(translated);
                if (translated.what == evNothing)
                    clearEvent(event);
                return;
            }
        }

        TMenuBar::handleEvent(event);
    }

private:
    char matchingTopLevelMnemonic(char typed) noexcept
    {
        if (typed == 0)
            return 0;

        const char upperTyped = static_cast<char>(
            std::toupper(static_cast<unsigned char>(typed)));
        return findItem(upperTyped) != nullptr ? upperTyped : 0;
    }
};

TMenuBar* buildMenuBar(TRect bounds, tvweather::Language language)
{
    const auto& strings = tvweather::uiStrings(language);
    const TKey weatherMenuKey = language == tvweather::Language::Portuguese
        ? kbAltM
        : kbAltW;
    bounds.b.y = bounds.a.y + 1;

    return new TopLevelMnemonicMenuBar(bounds,
        *new TSubMenu(strings.weatherMenu, weatherMenuKey) +
            *new TMenuItem(strings.pickLocation,    cmOpenLocationPicker, kbF3, hcNoContext, "F3") +
            *new TMenuItem(strings.activeWarnings,  cmShowWarnings,       kbF4, hcNoContext, "F4") +
            *new TMenuItem(strings.uvIndex,         cmShowUv,             kbF5, hcNoContext, "F5") +
            *new TMenuItem(strings.seaConditions,   cmShowSea,            kbF6, hcNoContext, "F6") +
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
            *new TStatusItem(strings.statusExit,    kbCtrlQ, cmQuit) +
            *new TStatusItem(strings.statusRefresh, kbF2,    cmRefresh) +
            *new TStatusItem(0, kbF10, cmMenu));
}

void refreshWindowLanguage(TView* view, void* languagePtr)
{
    const auto language = *static_cast<tvweather::Language*>(languagePtr);
    if (auto* forecastWindow = dynamic_cast<ForecastWindow*>(view))
        forecastWindow->setLanguage(language);
    else if (auto* warningsWindow = dynamic_cast<WarningsWindow*>(view))
        warningsWindow->setLanguage(language);
    else if (auto* uvWindow = dynamic_cast<UvWindow*>(view))
        uvWindow->setLanguage(language);
    else if (auto* seaWindow = dynamic_cast<SeaWindow*>(view))
        seaWindow->setLanguage(language);
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

    if (locations_.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

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
        case cmShowUv:
            showUv();
            clearEvent(event);
            break;
        case cmShowSea:
            showSea();
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
    if (forecast.days.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }
    insertWindow(new ForecastWindow(forecast, weatherTypes_, language_));
}

void TvWeatherApp::showWarnings()
{
    const auto warnings = IpmaClient::fetchWarnings();
    insertWindow(new WarningsWindow(warnings, language_));
}

void TvWeatherApp::showUv()
{
    if (locations_.empty())
        return;

    auto* dlg = new LocationPickerDialog(locations_, language_);
    const ushort result = TProgram::deskTop->execView(dlg);
    if (result == cmOK) {
        const auto loc = dlg->selectedLocation();
        delete dlg;
        if (loc.globalIdLocal == 0)
            return;
        const auto entries = IpmaClient::fetchUv();
        insertWindow(new UvWindow(entries, loc.globalIdLocal, loc.local, language_));
    } else {
        delete dlg;
    }
}

void TvWeatherApp::showSea()
{
    const auto seaLocations = IpmaClient::fetchSeaLocations();
    if (seaLocations.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    // Build IpmaLocation vector from sea locations for the picker dialog.
    std::vector<IpmaLocation> pickerLocations;
    pickerLocations.reserve(seaLocations.size());
    for (const auto& sl : seaLocations) {
        IpmaLocation loc;
        loc.globalIdLocal = sl.globalIdLocal;
        loc.local         = sl.local;
        pickerLocations.push_back(std::move(loc));
    }

    auto* dlg = new LocationPickerDialog(pickerLocations, language_);
    const ushort result = TProgram::deskTop->execView(dlg);
    if (result == cmOK) {
        const auto loc = dlg->selectedLocation();
        delete dlg;
        if (loc.globalIdLocal == 0)
            return;
        const auto forecast = IpmaClient::fetchSeaForecast(loc.globalIdLocal, loc.local);
        if (forecast.days.empty()) {
            messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
            return;
        }
        insertWindow(new SeaWindow(forecast, language_));
    } else {
        delete dlg;
    }
}
