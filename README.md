# tvweather

A C++ TurboVision starter project scaffolded around a small `tvweather` application.

## Layout

- `external/tvision`: vendored TurboVision submodule
- `include/tvweather`: public app headers
- `src`: application implementation
- `app`: executable entry point

## Prerequisites

- CMake 3.21 or newer
- A C++17 compiler
- A terminal environment supported by TurboVision

TurboVision handles its own platform-specific terminal linkage. On Linux, you may need the `ncursesw` development package installed before configuring the project. On Windows, the scaffold is set up for MSVC and TurboVision's Win32 console backend.

## Getting Started

Clone with submodules or initialize them after cloning:

```sh
git submodule update --init --recursive
```

Configure and build:

```sh
cmake --preset unix-debug
cmake --build --preset unix-debug
```

Run the starter application:

```sh
./build/unix-debug/bin/tvweather
```

## Windows Build

Configure and build with Visual Studio 2022:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-debug
```

The executable will be written under:

```powershell
build/windows-msvc-debug/bin/Debug/tvweather.exe
```

## Starter App

The scaffold opens a small TurboVision window on launch. Use:

- `Ctrl-Q` to quit
- `F2` to reopen the starter window
- `F10` to open the menu bar
