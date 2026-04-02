cmake_minimum_required(VERSION 3.21)

project(tvweather VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TV_BUILD_EXAMPLES OFF CACHE BOOL "Disable TurboVision example targets" FORCE)
set(TV_BUILD_TESTS OFF CACHE BOOL "Disable TurboVision test targets" FORCE)

add_subdirectory(external/tvision)

add_library(tvweather_core STATIC
    src/TvWeatherApp.cpp
)

target_compile_features(tvweather_core PUBLIC cxx_std_17)

target_include_directories(tvweather_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(tvweather_core
    PUBLIC
        tvision
)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(tvweather_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(tvweather_core PRIVATE /W4 /permissive-)
endif()

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
