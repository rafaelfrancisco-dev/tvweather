#pragma once
#include "ipma/Models.h"
#include <string>
#include <vector>

class IpmaClient {
public:
    // ── Network fetch + parse ─────────────────────────────────────────────
    static std::vector<IpmaLocation>    fetchLocations();
    static IpmaForecast                 fetchForecast(int globalIdLocal,
                                                      const std::string& localName);
    static std::vector<IpmaWarning>     fetchWarnings();
    static std::vector<IpmaWeatherType> fetchWeatherTypes();
    static std::vector<IpmaUvEntry>     fetchUv();
    static std::vector<IpmaSeaLocation> fetchSeaLocations();
    static IpmaSeaForecast              fetchSeaForecast(int globalIdLocal,
                                                         const std::string& localName);

    // ── Parse-only (exposed for unit tests) ──────────────────────────────
    static std::vector<IpmaLocation>    parseLocations(const std::string& json);
    static IpmaForecast                 parseForecast(const std::string& json,
                                                      int globalIdLocal,
                                                      const std::string& localName);
    // Filters out green (no-risk) entries; caller receives actionable warnings only.
    static std::vector<IpmaWarning>     parseWarnings(const std::string& json);
    static std::vector<IpmaWeatherType> parseWeatherTypes(const std::string& json);
    static std::vector<IpmaUvEntry>     parseUv(const std::string& json);
    static std::vector<IpmaSeaLocation> parseSeaLocations(const std::string& json);
    static std::vector<IpmaSeaForecastDay> parseSeaForecastDay(const std::string& json,
                                                                int globalIdLocal);

private:
    static std::string httpGet(const std::string& url);

    static constexpr const char* kBase = "https://api.ipma.pt";
};
