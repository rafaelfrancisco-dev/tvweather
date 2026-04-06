#define Uses_MsgBox
#define Uses_TButton
#define Uses_TColorDialog
#define Uses_TColorGroup
#define Uses_TColorItem
#define Uses_TScreen
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
#include "tvweather/ObservationsWindow.h"

#include <tvision/colorsel.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>

namespace {

class PaletteDialog final : public TColorDialog
{
public:
    PaletteDialog(const TPalette& palette,
                  const TPalette& originalPalette,
                  TColorGroup*    groups,
                  const char*     resetText) noexcept
        : TWindowInit(TWindow::initFrame),
          TColorDialog(nullptr, groups),
          originalPalette_(originalPalette)
    {
        insert(new TButton(TRect(24, 15, 34, 17), resetText, cmResetColors, bfNormal));
        TPalette initialPalette("", 0);
        initialPalette = palette;
        setData(&initialPalette);
    }

    void handleEvent(TEvent& event) override
    {
        if (event.what == evCommand && event.message.command == cmResetColors) {
            setData(&originalPalette_);
            clearEvent(event);
            return;
        }

        TColorDialog::handleEvent(event);
    }

private:
    TPalette originalPalette_;
};

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

tvweather::SavedPalette currentPaletteSettings()
{
    const TPalette& palette = TProgram::application->getPalette();

    tvweather::SavedPalette savedPalette;
    savedPalette.mode = TProgram::appPalette;

    const int size = palette[0];
    savedPalette.entries.reserve(size);
    for (int index = 1; index <= size; ++index)
        savedPalette.entries.push_back(palette[index]);

    return savedPalette;
}

bool samePalette(const tvweather::SavedPalette& left,
                 const tvweather::SavedPalette& right) noexcept
{
    return left.mode == right.mode && left.entries == right.entries;
}

TPalette toPalette(const tvweather::SavedPalette& palette)
{
    if (palette.entries.empty())
        return TPalette("", 0);

    std::vector<TColorAttr> entries;
    entries.reserve(palette.entries.size());
    for (const int entry : palette.entries)
        entries.push_back(static_cast<TColorAttr>(entry));
    return TPalette(entries.data(), static_cast<ushort>(entries.size()));
}

TColorGroup* createPaletteGroups()
{
    return &(
        *new TColorGroup("Desktop") +
            *new TColorItem("Color", 1) +

        *new TColorGroup("Menus") +
            *new TColorItem("Normal", 2) +
            *new TColorItem("Disabled", 3) +
            *new TColorItem("Shortcut", 4) +
            *new TColorItem("Selected", 5) +
            *new TColorItem("Selected disabled", 6) +
            *new TColorItem("Shortcut selected", 7) +

        *new TColorGroup("Blue windows") +
            *new TColorItem("Frame passive", 8) +
            *new TColorItem("Frame active", 9) +
            *new TColorItem("Frame icon", 10) +
            *new TColorItem("Scroll bar page", 11) +
            *new TColorItem("Scroll bar controls", 12) +
            *new TColorItem("Scroller text", 13) +
            *new TColorItem("Scroller selected", 14) +

        *new TColorGroup("Cyan windows") +
            *new TColorItem("Frame passive", 16) +
            *new TColorItem("Frame active", 17) +
            *new TColorItem("Frame icon", 18) +
            *new TColorItem("Scroll bar page", 19) +
            *new TColorItem("Scroll bar controls", 20) +
            *new TColorItem("Scroller text", 21) +
            *new TColorItem("Scroller selected", 22) +

        *new TColorGroup("Gray windows") +
            *new TColorItem("Frame passive", 24) +
            *new TColorItem("Frame active", 25) +
            *new TColorItem("Frame icon", 26) +
            *new TColorItem("Scroll bar page", 27) +
            *new TColorItem("Scroll bar controls", 28) +
            *new TColorItem("Scroller text", 29) +
            *new TColorItem("Scroller selected", 30) +

        *new TColorGroup("Gray dialogs") +
            *new TColorItem("Frame/background", 33) +
            *new TColorItem("Frame icons", 34) +
            *new TColorItem("Scroll bar page", 35) +
            *new TColorItem("Scroll bar icons", 36) +
            *new TColorItem("Static text", 37) +
            *new TColorItem("Label normal", 38) +
            *new TColorItem("Label selected", 39) +
            *new TColorItem("Label shortcut", 40) +
            *new TColorItem("Button normal", 41) +
            *new TColorItem("Button default", 42) +
            *new TColorItem("Button selected", 43) +
            *new TColorItem("Button disabled", 44) +
            *new TColorItem("Button shortcut", 45) +
            *new TColorItem("Button shadow", 46) +
            *new TColorItem("Cluster normal", 47) +
            *new TColorItem("Cluster selected", 48) +
            *new TColorItem("Cluster shortcut", 49) +
            *new TColorItem("Input normal", 50) +
            *new TColorItem("Input selected", 51) +
            *new TColorItem("Input arrow", 52) +
            *new TColorItem("History button", 53) +
            *new TColorItem("History sides", 54) +
            *new TColorItem("History bar page", 55) +
            *new TColorItem("History bar icons", 56) +
            *new TColorItem("List normal", 57) +
            *new TColorItem("List focused", 58) +
            *new TColorItem("List selected", 59) +
            *new TColorItem("List divider", 60) +
            *new TColorItem("Information pane", 61) +

        *new TColorGroup("Blue dialogs") +
            *new TColorItem("Frame/background", 65) +
            *new TColorItem("Frame icons", 66) +
            *new TColorItem("Scroll bar page", 67) +
            *new TColorItem("Scroll bar icons", 68) +
            *new TColorItem("Static text", 69) +
            *new TColorItem("Label normal", 70) +
            *new TColorItem("Label selected", 71) +
            *new TColorItem("Label shortcut", 72) +
            *new TColorItem("Button normal", 73) +
            *new TColorItem("Button default", 74) +
            *new TColorItem("Button selected", 75) +
            *new TColorItem("Button disabled", 76) +
            *new TColorItem("Button shortcut", 77) +
            *new TColorItem("Button shadow", 78) +
            *new TColorItem("Cluster normal", 79) +
            *new TColorItem("Cluster selected", 80) +
            *new TColorItem("Cluster shortcut", 81) +
            *new TColorItem("Input normal", 82) +
            *new TColorItem("Input selected", 83) +
            *new TColorItem("Input arrow", 84) +
            *new TColorItem("History button", 85) +
            *new TColorItem("History sides", 86) +
            *new TColorItem("History bar page", 87) +
            *new TColorItem("History bar icons", 88) +
            *new TColorItem("List normal", 89) +
            *new TColorItem("List focused", 90) +
            *new TColorItem("List selected", 91) +
            *new TColorItem("List divider", 92) +
            *new TColorItem("Information pane", 93) +

        *new TColorGroup("Cyan dialogs") +
            *new TColorItem("Frame/background", 97) +
            *new TColorItem("Frame icons", 98) +
            *new TColorItem("Scroll bar page", 99) +
            *new TColorItem("Scroll bar icons", 100) +
            *new TColorItem("Static text", 101) +
            *new TColorItem("Label normal", 102) +
            *new TColorItem("Label selected", 103) +
            *new TColorItem("Label shortcut", 104) +
            *new TColorItem("Button normal", 105) +
            *new TColorItem("Button default", 106) +
            *new TColorItem("Button selected", 107) +
            *new TColorItem("Button disabled", 108) +
            *new TColorItem("Button shortcut", 109) +
            *new TColorItem("Button shadow", 110) +
            *new TColorItem("Cluster normal", 111) +
            *new TColorItem("Cluster selected", 112) +
            *new TColorItem("Cluster shortcut", 113) +
            *new TColorItem("Input normal", 114) +
            *new TColorItem("Input selected", 115) +
            *new TColorItem("Input arrow", 116) +
            *new TColorItem("History button", 117) +
            *new TColorItem("History sides", 118) +
            *new TColorItem("History bar page", 119) +
            *new TColorItem("History bar icons", 120) +
            *new TColorItem("List normal", 121) +
            *new TColorItem("List focused", 122) +
            *new TColorItem("List selected", 123) +
            *new TColorItem("List divider", 124) +
            *new TColorItem("Information pane", 125));
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

const IpmaLocation* findLocation(const std::vector<IpmaLocation>& locations,
                                 int                             globalIdLocal)
{
    const auto it = std::find_if(locations.begin(), locations.end(),
                                 [globalIdLocal](const IpmaLocation& location) {
                                     return location.globalIdLocal == globalIdLocal;
                                 });
    return it != locations.end() ? &*it : nullptr;
}

std::vector<IpmaWarning> filterWarningsForArea(const std::vector<IpmaWarning>& warnings,
                                               const std::string& areaCode)
{
    if (areaCode.empty())
        return {};

    std::vector<IpmaWarning> filtered;
    filtered.reserve(warnings.size());
    for (const auto& warning : warnings) {
        if (warning.idAreaAviso == areaCode)
            filtered.push_back(warning);
    }
    return filtered;
}

double toRadians(double degrees)
{
    return degrees * 3.14159265358979323846 / 180.0;
}

double distanceKm(double latitudeA, double longitudeA,
                  double latitudeB, double longitudeB)
{
    constexpr double earthRadiusKm = 6371.0;

    const double latA = toRadians(latitudeA);
    const double lonA = toRadians(longitudeA);
    const double latB = toRadians(latitudeB);
    const double lonB = toRadians(longitudeB);

    const double deltaLat = latB - latA;
    const double deltaLon = lonB - lonA;
    const double sinLat = std::sin(deltaLat / 2.0);
    const double sinLon = std::sin(deltaLon / 2.0);
    const double a = sinLat * sinLat
        + std::cos(latA) * std::cos(latB) * sinLon * sinLon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return earthRadiusKm * c;
}

const IpmaStation* findNearestStation(const IpmaLocation&              selectedLocation,
                                      const std::vector<IpmaStation>& stations)
{
    if (!selectedLocation.latitude.has_value() || !selectedLocation.longitude.has_value())
        return nullptr;

    const IpmaStation* nearest = nullptr;
    double bestDistance = std::numeric_limits<double>::max();

    for (const auto& station : stations) {
        if (!station.latitude.has_value() || !station.longitude.has_value())
            continue;

        const double distance = distanceKm(*selectedLocation.latitude,
                                           *selectedLocation.longitude,
                                           *station.latitude,
                                           *station.longitude);
        if (distance < bestDistance) {
            bestDistance = distance;
            nearest = &station;
        }
    }

    return nearest;
}

const IpmaSeaLocation* findNearestSeaLocation(const IpmaLocation&                    selectedLocation,
                                              const std::vector<IpmaSeaLocation>& seaLocations)
{
    if (!selectedLocation.latitude.has_value() || !selectedLocation.longitude.has_value())
        return nullptr;

    const IpmaSeaLocation* nearest = findSeaLocation(seaLocations, selectedLocation.globalIdLocal);
    double bestDistance = nearest != nullptr ? 0.0 : std::numeric_limits<double>::max();

    for (const auto& seaLocation : seaLocations) {
        if (!seaLocation.latitude.has_value() || !seaLocation.longitude.has_value())
            continue;

        const double distance = distanceKm(*selectedLocation.latitude,
                                           *selectedLocation.longitude,
                                           *seaLocation.latitude,
                                           *seaLocation.longitude);
        if (distance < bestDistance) {
            bestDistance = distance;
            nearest = &seaLocation;
        }
    }

    return nearest;
}

double distanceToSeaLocationKm(const IpmaLocation& selectedLocation,
                               const IpmaSeaLocation& seaLocation)
{
    if (!selectedLocation.latitude.has_value() || !selectedLocation.longitude.has_value() ||
        !seaLocation.latitude.has_value() || !seaLocation.longitude.has_value())
        return 0.0;

    return distanceKm(*selectedLocation.latitude,
                      *selectedLocation.longitude,
                      *seaLocation.latitude,
                      *seaLocation.longitude);
}

std::string buildSeaClosestLocationMessage(tvweather::Language language,
                                           const std::string& selectedLocationName,
                                           const std::string& seaLocationName,
                                           double             distanceKmValue)
{
    const auto& strings = tvweather::uiStrings(language);
    char distanceBuffer[32];
    std::snprintf(distanceBuffer, sizeof(distanceBuffer), "%.1f km", distanceKmValue);

    return std::string(strings.seaClosestNotice)
        + "\n\n"
        + strings.seaClosestLocationLabel
        + ": "
        + seaLocationName
        + "\n"
        + strings.seaClosestDistanceLabel
        + " ("
        + selectedLocationName
        + "): "
        + distanceBuffer;
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
            newLine() +
            *new TMenuItem(strings.activeWarnings,  cmShowWarnings,       kbF4, hcNoContext, "F4") +
            *new TMenuItem(strings.uvIndex,         cmShowUv,             kbF5, hcNoContext, "F5") +
            *new TMenuItem(strings.seaConditions,   cmShowSea,            kbF6, hcNoContext, "F6") +
            *new TMenuItem(strings.obsMenuLabel,    cmShowObservations,   kbF7, hcNoContext, "F7") +
            newLine() +
            *new TMenuItem(strings.exitApp, cmQuit, kbCtrlQ, hcNoContext, "Ctrl-Q") +
        *new TSubMenu(strings.optionsMenu, kbAltO) +
            *new TMenuItem(strings.preferences, cmOpenOptions, kbNoKey, hcNoContext) +
            *new TMenuItem(strings.colors, cmOpenColors, kbNoKey, hcNoContext));
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
    else if (auto* obsWindow = dynamic_cast<ObservationsWindow*>(view))
        obsWindow->setLanguage(language);
}

bool isLocationDependentWindow(TView* view)
{
    return dynamic_cast<ForecastWindow*>(view) != nullptr
        || dynamic_cast<UvWindow*>(view) != nullptr
        || dynamic_cast<SeaWindow*>(view) != nullptr
        || dynamic_cast<ObservationsWindow*>(view) != nullptr;
}

void collectLocationDependentWindows(TView* view, void* windowsPtr)
{
    if (!isLocationDependentWindow(view))
        return;

    auto& windows = *static_cast<std::vector<TWindow*>*>(windowsPtr);
    windows.push_back(static_cast<TWindow*>(view));
}

} // namespace

TvWeatherApp::TvWeatherApp() :
    TProgInit(&TvWeatherApp::initStatusLine,
              &TvWeatherApp::initMenuBar,
              &TvWeatherApp::initDeskTop)
{
    const auto settings = tvweather::loadSettings();
    language_ = settings.language;
    originalPalette_ = currentPaletteSettings();
    palette_ = settings.palette;
    selectedLocation_ = settings.selectedLocation;
    applySavedPalette(palette_);
    rebuildStatusLine();
    rebuildMenuBar();
    updateLocationCommandState();
    locations_          = IpmaClient::fetchLocations();
    weatherTypes_       = IpmaClient::fetchWeatherTypes();
    windTypes_          = IpmaClient::fetchWindTypes();
    windDirectionTypes_ = IpmaClient::fetchWindDirectionTypes();
    stations_           = IpmaClient::fetchStations();

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
        case cmShowObservations:
            showObservations();
            clearEvent(event);
            break;
        case cmOpenOptions:
            showOptions();
            clearEvent(event);
            break;
        case cmOpenColors:
            showColors();
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

void TvWeatherApp::applySavedPalette(const std::optional<tvweather::SavedPalette>& palette)
{
    if (!palette.has_value())
        return;
    if (palette->mode != TProgram::appPalette)
        return;

    TPalette& current = getPalette();
    const int expectedSize = current[0];
    if (static_cast<int>(palette->entries.size()) != expectedSize)
        return;

    for (int index = 0; index < expectedSize; ++index)
        current[index + 1] = static_cast<uchar>(palette->entries[index]);

    setScreenMode(TScreen::screenMode);
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
    settings.palette = palette_;
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

    closeLocationDependentWindows();
    selectedLocation_.reset();
    saveSettings();
    rebuildStatusLine();
    updateLocationCommandState();
}

void TvWeatherApp::closeLocationDependentWindows()
{
    std::vector<TWindow*> windows;
    TProgram::deskTop->forEach(collectLocationDependentWindows, &windows);

    for (TWindow* window : windows)
        if (window != nullptr)
            window->close();
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

void TvWeatherApp::showColors()
{
    TPalette currentPalette("", 0);
    currentPalette = getPalette();

    TView* view = validView(
        new PaletteDialog(currentPalette,
                          toPalette(originalPalette_),
                          createPaletteGroups(),
                          tvweather::uiStrings(language_).reset));
    if (view == nullptr)
        return;

    auto* dialog = static_cast<PaletteDialog*>(view);

    if (TProgram::deskTop->execView(dialog) == cmOK) {
        TPalette editedPalette("", 0);
        dialog->getData(&editedPalette);
        getPalette() = editedPalette;
        setScreenMode(TScreen::screenMode);
        const auto currentSettings = currentPaletteSettings();
        if (samePalette(currentSettings, originalPalette_))
            palette_.reset();
        else
            palette_ = currentSettings;
        saveSettings();
    }

    destroy(dialog);
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
    const auto* location = findLocation(locations_, globalIdLocal);
    const std::string resolvedLocalName = location != nullptr ? location->local : localName;
    const std::string areaCode = location != nullptr ? location->idAreaAviso : std::string();

    const auto forecast = IpmaClient::fetchAggregateForecast(globalIdLocal, resolvedLocalName);
    if (forecast.days.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const auto warnings = filterWarningsForArea(IpmaClient::fetchWarnings(), areaCode);

    insertWindow(new ForecastWindow(forecast,
                                    weatherTypes_,
                                    windTypes_,
                                    windDirectionTypes_,
                                    warnings,
                                    areaCode,
                                    language_));
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

void TvWeatherApp::showObservations()
{
    if (!ensureSelectedLocation())
        return;

    const auto* selectedLocation = findLocation(locations_, selectedLocation_->globalIdLocal);
    if (selectedLocation == nullptr) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    if (stations_.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const auto* station = findNearestStation(*selectedLocation, stations_);
    if (station == nullptr) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const double stationDistKm = (selectedLocation->latitude.has_value()
                                  && selectedLocation->longitude.has_value()
                                  && station->latitude.has_value()
                                  && station->longitude.has_value())
        ? distanceKm(*selectedLocation->latitude, *selectedLocation->longitude,
                     *station->latitude,          *station->longitude)
        : 0.0;

    const auto obs = IpmaClient::fetchObservation(station->idEstacao, station->localEstacao);

    insertWindow(new ObservationsWindow(obs,
                                        stationDistKm,
                                        selectedLocation_->local,
                                        language_));
}

void TvWeatherApp::showSea()
{
    if (!ensureSelectedLocation())
        return;

    const auto seaLocations = IpmaClient::fetchSeaLocations();
    if (seaLocations.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const auto* selectedLocation = findLocation(locations_, selectedLocation_->globalIdLocal);
    if (selectedLocation == nullptr) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const auto* seaLocation = findNearestSeaLocation(*selectedLocation, seaLocations);
    if (seaLocation == nullptr) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    const double seaDistanceKm = distanceToSeaLocationKm(*selectedLocation, *seaLocation);

    const auto forecast = IpmaClient::fetchSeaForecast(seaLocation->globalIdLocal,
                                                       seaLocation->local);
    if (forecast.days.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        return;
    }

    messageBox(buildSeaClosestLocationMessage(language_,
                                              selectedLocation->local,
                                              seaLocation->local,
                                              seaDistanceKm).c_str(),
               mfInformation | mfOKButton);
    insertWindow(new SeaWindow(forecast, language_));
}
