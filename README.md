# tvweather

A terminal weather application for Portugal, built with C++ and [TurboVision](https://github.com/magiblot/tvision). Data is sourced from the [IPMA open API](https://api.ipma.pt) (Instituto Português do Mar e da Atmosfera).

```
╔══════════════════════════════════════════════════════════════════════════════╗
║ Weather                                                          Options      ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  ╔════════════════ Lisboa - 5-Day Forecast ═════════════════╗                ║
║  ║ 04-02 11/23   0% │ Details 1/5                           ║                ║
║  ║ 04-03 10/26   0% │ 2026-04-02                            ║                ║
║  ║ 04-04 14/26   0% │ Partly cloudy                         ║                ║
║  ║ 04-05 13/28   0% │ Temperature: 11° / 23°                ║                ║
║  ║ 04-06 11/24  35% │ Rain: 0%                              ║                ║
║  ║                  │ Rain risk: Low                        ║                ║
║  ║                  │ Wind: N / 2                           ║                ║
║  ║                  │ Wind strength: Moderate               ║                ║
║  ╚═════════════════════════════════════════════════════════╝                 ║
╠══════════════════════════════════════════════════════════════════════════════╣
║ ~Ctrl-Q~ Exit  ~F2~ Refresh                                                  ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

## Features

- **Location picker with search** — all IPMA forecast locations (mainland Portugal + Madeira + Azores) with a live filter field to narrow by name
- **Selected location state** — the last picked location becomes the active location, is shown in the status bar, is reused by location-based menus, and persists across restarts until cleared
- **5-day forecast** — split view with a selectable day list plus a detail pane for min/max temperature, precipitation probability, wind direction, wind class, and localized condition text
- **Active warnings** — yellow, orange and red meteorological warnings, scrollable, with type and area
- **UV Index** — daily UV index forecast per location with time intervals (e.g. 13h–15h)
- **Sea conditions** — 3-day wave height, wave period, wave direction, and sea surface temperature for 12 coastal locations
- **Refresh** — re-fetch data for the active window at any time with F2
- **Error notifications** — message box shown when API calls fail or return empty data
- **Options dialog** — dedicated settings window for app preferences
- **Language switching** — English/Portuguese UI toggle persisted across restarts
- **Clear location** — remove the active location to force a new pick later
- Multiple forecast/UV/sea windows can be open simultaneously (tiled or overlapping)

## Requirements

| Requirement | Version |
|---|---|
| CMake | 3.21+ |
| C++ compiler | C++17 (AppleClang, Clang, GCC, MSVC) |
| curl | Available at runtime for API calls |
| Internet | Required at launch and for all data fetches |

On Linux, install the `ncursesw` development package before configuring:
```sh
# Debian / Ubuntu
sudo apt install libncursesw5-dev
```

## Build

Clone with the TurboVision submodule:
```sh
git clone --recurse-submodules <repo-url>
# or after cloning:
git submodule update --init --recursive
```

Configure and build (macOS / Linux):
```sh
cmake --preset unix-debug
cmake --build --preset unix-debug
```

Run:
```sh
./build/unix-debug/bin/tvweather
```

### Windows (MSVC)

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
.\build\windows-msvc-debug\bin\Debug\tvweather.exe
```

### Manual configuration (no presets)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/bin/tvweather
```

## Usage

The app fetches the location list on startup. If no location is already saved, it opens the location picker.

| Key | Action |
|---|---|
| `F10` | Open the menu bar |
| `W` (in menu) | Open the Weather menu |
| `O` (in menu) | Open the Options menu |
| Arrow keys | Navigate lists, forecast days, and windows |
| `Enter` | Confirm selection |
| `Esc` | Cancel / close dialog |
| `F2` | Refresh data for the active window |
| `F8` | Clear the active location |
| `Ctrl-Q` | Quit |

In forecast and sea windows, the day list on the left is a standard Turbo Vision list view. Use `Up` / `Down`, `PgUp` / `PgDn`, `Home`, and `End` to move through the available forecast days.

### Selected location flow

1. Pick a location once with `F3` or at first launch.
2. The selected location becomes the active location and is shown in the status bar.
3. `Forecast...` reuses that active location directly.
4. `UV Index...` reuses that active location directly.
5. `Sea Conditions...` reuses it when the selected `globalIdLocal` exists in the sea dataset; otherwise it asks for a coastal location.
6. `Clear Location` removes the active location from memory and from the saved config so the next location-dependent action can ask again.

**Weather menu items:**

| Item | Key | Description |
|---|---|---|
| Forecast… | F3 | Opens the forecast for the active location; if none is selected, opens the location picker first |
| Active Warnings | F4 | Fetches and displays current non-green meteorological warnings |
| UV Index… | F5 | Uses the active location; if none is selected, opens the location picker first |
| Sea Conditions… | F6 | Uses the active location when it has sea data; otherwise opens a coastal location picker |
| Exit | Ctrl-Q | Quit the application |

**Options menu items:**

| Item | Description |
|---|---|
| Preferences... | Opens a dedicated options dialog; currently used to change the UI language |

**Location picker search:** type any part of a location name to filter the list in real time. The search is case-insensitive. Press `Tab` or `Down` to move focus to the filtered list.

**Selected location:** the status bar shows the current active location. Clearing it removes the saved location from the config file, and the next location-dependent action prompts for a new one.

## Settings And Persistence

tvweather stores its config in:

- macOS / Linux: `~/.config/tvweather/config.json`
- Windows: `%APPDATA%/tvweather/config.json`

Current persisted fields:

- `language`
- `selectedLocation.globalIdLocal`
- `selectedLocation.local`

Example:

```json
{
  "language": "en",
  "selectedLocation": {
    "globalIdLocal": 1110600,
    "local": "Lisboa"
  }
}
```

If the saved location is missing from the latest IPMA location list, tvweather drops it and falls back to the picker flow.

> **Note on green warnings:** The IPMA API publishes baseline "green" entries for every location and hazard type regardless of risk. tvweather filters these out and only shows yellow, orange and red warnings.

## Project Structure

```
tvweather/
├── app/
│   └── main.cpp                  # Entry point
├── lib/                          # ipmalib — data layer (no TurboVision)
│   ├── include/ipma/
│   │   ├── Models.h              # Plain data structs
│   │   └── Client.h              # IpmaClient declaration
│   └── IpmaClient.cpp            # HTTP (curl) + JSON parsing
├── include/tvweather/            # UI layer headers
│   ├── Commands.h                # App command IDs
│   ├── Language.h                # UI language enum
│   ├── Localization.h            # App-owned translated strings
│   ├── Settings.h                # Persisted app settings
│   ├── TvWeatherApp.h
│   ├── OptionsDialog.h
│   ├── LocationPickerDialog.h
│   ├── ForecastWindow.h
│   ├── WarningsWindow.h
│   ├── UvWindow.h
│   └── SeaWindow.h
├── src/                          # UI layer implementation
│   ├── TvWeatherApp.cpp
│   ├── Localization.cpp
│   ├── Settings.cpp
│   ├── OptionsDialog.cpp
│   ├── LocationPickerDialog.cpp
│   ├── ForecastWindow.cpp
│   ├── WarningsWindow.cpp
│   ├── UvWindow.cpp
│   └── SeaWindow.cpp
├── tests/
│   ├── MiniTest.h                # Lightweight local test harness
│   ├── IpmaClientTest.cpp        # Unit tests for ipmalib
│   └── TvWeatherSupportTest.cpp  # Unit tests for settings/localization support
├── external/
│   └── tvision/                  # TurboVision submodule
└── CMakeLists.txt
```

### Dependency boundary

```
ipmalib  (nlohmann/json + curl, no TurboVision)
    └── tvweather_core  (TurboVision UI, no JSON)
            └── tvweather  (executable)
```

`ipmalib` has no TurboVision dependency and can be built and tested independently. UI components receive model structs by value — they perform no network calls.

## Testing

Unit tests cover the JSON parsing layer (`IpmaClient::parse*` methods) and the localization/settings helpers without making any network calls:

```sh
cd build/unix-debug   # or build/
ctest --output-on-failure
```

Tests include:
- Location parsing from the wrapped `{"data": [...]}` IPMA envelope
- Forecast parsing with numeric-string fields (`"tMin": "11.8"`) and null values
- Warning filtering (green entries excluded)
- Warning display fields (`awarenessTypeName`, `idAreaAviso`)
- Weather type lookup
- UV index entry parsing (location, UV value, time interval, date)
- Sea location parsing from the current top-level array payload
- Sea forecast day parsing from the current wrapped payload (`forecastDate` + `data`) with `waveHighMin` / `waveHighMax`
- Backward-compatible sea parsing for older wrapped location payloads and legacy sea day arrays
- Language selection/persistence (`en` / `pt`)
- Selected location load/save/clear behavior
- Localized weather description lookup
- Malformed / empty JSON resilience

## Turbo Vision Notes

For settings UI, tvweather uses the standard Turbo Vision pattern of a modal `TDialog` containing controls such as `TRadioButtons` and `TButton`, instead of putting individual toggles directly in the menu. This keeps the current language selector idiomatic and leaves room to add more settings to the same options window later.

The location picker search uses `TInputLine` at the top of the dialog. On each key event, `handleEvent` compares the input value to the previous filter and rebuilds the `TStringCollection` passed to `TListBox::newList()` when it changes. This avoids polling and keeps the list in sync with typing.

## IPMA API Endpoints

| Endpoint | Used for |
|---|---|
| `/open-data/distrits-islands.json` | Location list |
| `/open-data/forecast/meteorology/cities/daily/{id}.json` | 5-day forecast |
| `/open-data/forecast/warnings/warnings_www.json` | Active warnings |
| `/open-data/weather-type-classe.json` | Weather condition descriptions |
| `/open-data/forecast/meteorology/uv/uv.json` | UV index forecast |
| `/open-data/sea-locations.json` | Coastal location list |
| `/open-data/forecast/oceanography/daily/hp-daily-sea-forecast-day{0,1,2}.json` | 3-day sea forecast |

All requests are made via `curl` (blocking, 10 s timeout). Failures return empty results and show an error dialog; the app remains usable after a failed fetch.
