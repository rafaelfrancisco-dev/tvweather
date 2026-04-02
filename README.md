# tvweather

A terminal weather application for Portugal, built with C++ and [TurboVision](https://github.com/magiblot/tvision). Data is sourced from the [IPMA open API](https://api.ipma.pt) (Instituto Português do Mar e da Atmosfera).

```
╔══════════════════════════════════════════════════════════════════════════════╗
║ Weather                                                                      ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  ╔══════════════ Lisboa - 5-Day Forecast ═══════════════╗                    ║
║  ║ 04-02 11/23   0% │ Details 1/5                       ║                    ║
║  ║ 04-03 10/26   0% │ 2026-04-02                        ║                    ║
║  ║ 04-04 14/26   0% │ Partly cloudy                     ║                    ║
║  ║ 04-05 13/28   0% │ Temperature: 11° / 23°            ║                    ║
║  ║ 04-06 11/24  35% │ Rain: 0%                          ║                    ║
║  ║                  │ Rain risk: Low                    ║                    ║
║  ║                  │ Wind: N / 2                       ║                    ║
║  ║                  │ Wind strength: Moderate           ║                    ║
║  ╚═══════════════════════════════════════════════════════╝                    ║
╠══════════════════════════════════════════════════════════════════════════════╣
║ Ctrl-Q Exit                                                                  ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

## Features

- **Location picker** — all IPMA forecast locations (mainland Portugal + Madeira + Azores)
- **5-day forecast** — split view with a selectable day list plus a detail pane for min/max temperature, precipitation probability, wind direction, wind class, and localized condition text
- **Active warnings** — yellow, orange and red meteorological warnings, scrollable, with type and area
- **Options dialog** — dedicated settings window for app preferences
- **Language switching** — English/Portuguese UI toggle persisted across restarts
- Multiple forecast windows can be open simultaneously (tiled or overlapping)

## Requirements

| Requirement | Version |
|---|---|
| CMake | 3.21+ |
| C++ compiler | C++17 (AppleClang, Clang, GCC, MSVC) |
| curl | Available at runtime for API calls |
| Internet | Required at launch and when fetching warnings |

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

The app fetches the location list on startup, then opens the location picker.

| Key | Action |
|---|---|
| `F10` | Open the menu bar |
| `W` (in menu) | Open the Weather menu |
| `O` (in menu) | Open the Options menu |
| Arrow keys | Navigate lists, forecast days, and windows |
| `Enter` | Confirm selection |
| `Esc` | Cancel / close dialog |
| `Ctrl-Q` | Quit |

In forecast windows, the day list on the left is a standard Turbo Vision list view. Use `Up` / `Down`, `PgUp` / `PgDn`, `Home`, and `End` to move through the available forecast days.

**Weather menu items:**

| Item | Description |
|---|---|
| Pick Location… | Opens the location picker; selecting a city fetches and displays its 5-day forecast |
| Active Warnings | Fetches and displays current non-green meteorological warnings |
| Exit | Quit the application |

**Options menu items:**

| Item | Description |
|---|---|
| Preferences... | Opens a dedicated options dialog; currently used to change the UI language |

> **Note on green warnings:** The IPMA API publishes baseline "green" entries for every location and hazard type regardless of risk. tvweather filters these out and only shows yellow, orange and red warnings.

## Project Structure

```
tvweather/
├── app/
│   └── main.cpp                  # Entry point
├── lib/                          # ipmalib — data layer (no TurboVision)
│   ├── include/ipma/
│   │   ├── Models.h              # Plain data structs
│   │   └── Client.h             # IpmaClient declaration
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
│   └── WarningsWindow.h
├── src/                          # UI layer implementation
│   ├── TvWeatherApp.cpp
│   ├── Localization.cpp
│   ├── Settings.cpp
│   ├── OptionsDialog.cpp
│   ├── LocationPickerDialog.cpp
│   ├── ForecastWindow.cpp
│   └── WarningsWindow.cpp
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
- Language selection/persistence (`en` / `pt`)
- Localized weather description lookup
- Malformed / empty JSON resilience

## Turbo Vision Notes

For settings UI, tvweather uses the standard Turbo Vision pattern of a modal `TDialog` containing controls such as `TRadioButtons` and `TButton`, instead of putting individual toggles directly in the menu. This keeps the current language selector idiomatic and leaves room to add more settings to the same options window later.

## IPMA API Endpoints

| Endpoint | Used for |
|---|---|
| `/open-data/distrits-islands.json` | Location list |
| `/open-data/forecast/meteorology/cities/daily/{id}.json` | 5-day forecast |
| `/open-data/forecast/warnings/warnings_www.json` | Active warnings |
| `/open-data/weather-type-classe.json` | Weather condition descriptions |

All requests are made via `curl` (blocking, 10 s timeout). Failures return empty results; the UI handles this gracefully.
