cmake_minimum_required(VERSION 3.21)

project(tvweather VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# Fetch doctest header only — avoids processing their outdated CMakeLists.txt
file(DOWNLOAD
    https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    ${CMAKE_BINARY_DIR}/doctest/doctest/doctest.h
    TLS_VERIFY ON
    STATUS doctest_download_status
)
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_BINARY_DIR}/doctest)

set(TV_BUILD_EXAMPLES OFF CACHE BOOL "Disable TurboVision example targets" FORCE)
set(TV_BUILD_TESTS OFF CACHE BOOL "Disable TurboVision test targets" FORCE)

add_subdirectory(external/tvision)

# ── ipmalib: pure data layer, no TurboVision ─────────────────────────────────
add_library(ipmalib STATIC
    lib/IpmaClient.cpp
)

target_compile_features(ipmalib PUBLIC cxx_std_17)

target_include_directories(ipmalib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/lib/include
)

target_link_libraries(ipmalib
    PUBLIC
        nlohmann_json::nlohmann_json
)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(ipmalib PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(ipmalib PRIVATE /W4 /permissive-)
endif()

# ── tvweather_core: TurboVision UI layer ──────────────────────────────────────
add_library(tvweather_core STATIC
    src/TvWeatherApp.cpp
    src/LocationPickerDialog.cpp
    src/ForecastWindow.cpp
    src/WarningsWindow.cpp
)

target_compile_features(tvweather_core PUBLIC cxx_std_17)

target_include_directories(tvweather_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(tvweather_core
    PUBLIC
        tvision
        ipmalib
)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(tvweather_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(tvweather_core PRIVATE /W4 /permissive-)
endif()

# ── ipmalib unit tests ────────────────────────────────────────────────────────
enable_testing()

add_executable(ipmalib_tests
    tests/IpmaClientTest.cpp
)
target_link_libraries(ipmalib_tests
    PRIVATE
        ipmalib
        doctest
)
if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(ipmalib_tests PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(ipmalib_tests PRIVATE /W4 /permissive-)
endif()
add_test(NAME ipmalib_tests COMMAND ipmalib_tests)

# ── tvweather: executable ─────────────────────────────────────────────────────
add_executable(tvweather
    app/main.cpp
)

target_link_libraries(tvweather
    PRIVATE
        tvweather_core
)

set_target_properties(tvweather PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
