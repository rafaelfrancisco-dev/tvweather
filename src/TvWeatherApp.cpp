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

#include <algorithm>
#include <cctype>
#include <optional>

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

class LocationStatusLine final : public TStatusLine
{
public:
    LocationStatusLine(const TRect& bounds,
                       TStatusDef&   defs,
                       std::string   locationHint) noexcept
        : TStatusLine(bounds, defs),
          locationHint_(std::move(locationHint))
    {
    }

    const char* hint(ushort) override
    {
        return locationHint_.c_str();
    }

private:
    std::string locationHint_;
};

std::string buildLocationHint(tvweather::Language language,
                              const std::optional<tvweather::SavedLocation>& location)
{
    const auto& strings = tvweather::uiStrings(language);
    if (!location.has_value())
        return strings.statusNoLocation;
    return std::string(strings.statusLocationPrefix) + location->local;
}

tvweather::SavedLocation toSavedLocation(const IpmaLocation& location)
{
    return {location.globalIdLocal, location.local};
}

const IpmaSeaLocation* findSeaLocation(const std::vector<IpmaSeaLocation>& locations,
                                       int                                 globalIdLocal)
{
    const auto it = std::find_if(locations.begin(), locations.end(),
                                 [globalIdLocal](const IpmaSeaLocation& location) {
                                     return location.globalIdLocal == globalIdLocal;
                                 });
    return it != locations.end() ? &*it : nullptr;
}

TMenuBar* buildMenuBar(TRect bounds, tvweather::Language language)
{
    const auto& strings = tvweather::uiStrings(language);
    const TKey weatherMenuKey = language == tvweather::Language::Portuguese
        ? kbAltM
        : kbAltW;
    bounds.b.y = bounds.a.y + 1;

    return new TopLevelMnemonicMenuBar(bounds,
        *new TSubMenu(strings.weatherMenu, weatherMenuKey) +
            *new TMenuItem(strings.pickLocation,    cmShowForecast,       kbF3, hcNoContext, "F3") +
            *new TMenuItem(strings.activeWarnings,  cmShowWarnings,       kbF4, hcNoContext, "F4") +
            *new TMenuItem(strings.uvIndex,         cmShowUv,             kbF5, hcNoContext, "F5") +
            *new TMenuItem(strings.seaConditions,   cmShowSea,            kbF6, hcNoContext, "F6") +
            newLine() +
            *new TMenuItem(strings.exitApp, cmQuit, kbCtrlQ, hcNoContext, "Ctrl-Q") +
        *new TSubMenu(strings.optionsMenu, kbAltO) +
            *new TMenuItem(strings.preferences, cmOpenOptions, kbNoKey, hcNoContext));
}

TStatusLine* buildStatusLine(TRect                                     bounds,
                             tvweather::Language                        language,
                             const std::optional<tvweather::SavedLocation>& location)
{
    const auto& strings = tvweather::uiStrings(language);
    bounds.a.y = bounds.b.y - 1;

    return new LocationStatusLine(bounds,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem(strings.statusExit,    kbCtrlQ, cmQuit) +
            *new TStatusItem(strings.statusRefresh, kbF2,    cmRefresh) +
            *new TStatusItem(strings.statusClearLocation, kbF8, cmClearLocation) +
            *new TStatusItem(0, kbF10, cmMenu),
        buildLocationHint(language, location));
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
    const auto settings = tvweather::loadSettings();
    language_ = settings.language;
    selectedLocation_ = settings.selectedLocation;
    rebuildStatusLine();
    rebuildMenuBar();
    updateLocationCommandState();
    locations_    = IpmaClient::fetchLocations();
    weatherTypes_ = IpmaClient::fetchWeatherTypes();

    if (locations_.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    if (selectedLocation_.has_value()) {
        const auto it = std::find_if(locations_.begin(), locations_.end(),
                                     [this](const IpmaLocation& location) {
                                         return location.globalIdLocal
                                             == selectedLocation_->globalIdLocal;
                                     });

        if (it != locations_.end()) {
            selectedLocation_->local = it->local;
            saveSettings();
            rebuildStatusLine();
        } else
            clearSelectedLocation();
    }

    if (!selectedLocation_.has_value())
        showLocationPicker();
}

void TvWeatherApp::handleEvent(TEvent& event)
{
    TApplication::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
        case cmShowForecast:
            showForecast();
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
        case cmClearLocation:
            clearSelectedLocation();
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
    return buildStatusLine(bounds, tvweather::Language::English, std::nullopt);
}

void TvWeatherApp::refreshLanguage(tvweather::Language language)
{
    if (language_ == language)
        return;

    language_ = language;
    saveSettings();
    rebuildStatusLine();
    rebuildMenuBar();
    updateLocationCommandState();
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

    TProgram::statusLine = buildStatusLine(getExtent(), language_, selectedLocation_);
    insert(TProgram::statusLine);
}

void TvWeatherApp::saveSettings() const
{
    tvweather::AppSettings settings;
    settings.language = language_;
    settings.selectedLocation = selectedLocation_;
    tvweather::saveSettings(settings);
}

void TvWeatherApp::selectLocation(const tvweather::SavedLocation& location)
{
    selectedLocation_ = location;
    saveSettings();
    rebuildStatusLine();
    updateLocationCommandState();
}

void TvWeatherApp::clearSelectedLocation()
{
    if (!selectedLocation_.has_value())
        return;

    selectedLocation_.reset();
    saveSettings();
    rebuildStatusLine();
    updateLocationCommandState();
}

void TvWeatherApp::updateLocationCommandState()
{
    if (selectedLocation_.has_value())
        enableCommand(cmClearLocation);
    else
        disableCommand(cmClearLocation);

    if (TProgram::menuBar != nullptr)
        message(TProgram::menuBar, evBroadcast, cmCommandSetChanged, nullptr);
    if (TProgram::statusLine != nullptr)
        message(TProgram::statusLine, evBroadcast, cmCommandSetChanged, nullptr);
}

std::optional<tvweather::SavedLocation>
TvWeatherApp::pickLocation(const std::vector<IpmaLocation>& locations)
{
    if (locations.empty())
        return std::nullopt;

    auto* dialog = new LocationPickerDialog(locations, language_);
    const ushort result = TProgram::deskTop->execView(dialog);

    std::optional<tvweather::SavedLocation> selected;
    if (result == cmOK) {
        const auto location = dialog->selectedLocation();
        if (location.globalIdLocal != 0)
            selected = toSavedLocation(location);
    }

    delete dialog;
    return selected;
}

bool TvWeatherApp::ensureSelectedLocation()
{
    if (selectedLocation_.has_value())
        return true;

    if (const auto location = pickLocation(locations_)) {
        selectLocation(*location);
        return true;
    }

    return false;
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
    if (const auto location = pickLocation(locations_)) {
        selectLocation(*location);
        showForecast(location->globalIdLocal, location->local);
    }
}

void TvWeatherApp::showForecast()
{
    if (!ensureSelectedLocation())
        return;

    showForecast(selectedLocation_->globalIdLocal, selectedLocation_->local);
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
    if (!ensureSelectedLocation())
        return;

    const auto entries = IpmaClient::fetchUv();
    insertWindow(new UvWindow(entries,
                              selectedLocation_->globalIdLocal,
                              selectedLocation_->local,
                              language_));
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

    tvweather::SavedLocation location;
    bool hasLocation = false;

    if (selectedLocation_.has_value()) {
        if (const auto* seaLocation = findSeaLocation(seaLocations,
                                                      selectedLocation_->globalIdLocal)) {
            location = {seaLocation->globalIdLocal, seaLocation->local};
            hasLocation = true;
        }
    }

    if (!hasLocation) {
        if (const auto selected = pickLocation(pickerLocations)) {
            location = *selected;
            selectLocation(location);
            hasLocation = true;
        }
    }

    if (!hasLocation)
        return;

    const auto forecast = IpmaClient::fetchSeaForecast(location.globalIdLocal, location.local);
    if (forecast.days.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    insertWindow(new SeaWindow(forecast, language_));
}
