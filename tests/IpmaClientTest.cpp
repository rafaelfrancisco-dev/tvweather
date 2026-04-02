#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "ipma/Client.h"

// ── Fixture JSON strings (captured from real API responses) ──────────────────

static const char* kLocationsJson = R"({
    "owner": "IPMA",
    "country": "PT",
    "data": [
        {"globalIdLocal": 1110600, "local": "Lisboa",  "idRegiao": 1, "idAreaAviso": "LIS"},
        {"globalIdLocal": 1131200, "local": "Porto",   "idRegiao": 1, "idAreaAviso": "PRT"},
        {"globalIdLocal": 2310300, "local": "Funchal", "idRegiao": 2, "idAreaAviso": "MAD"}
    ]
})";

static const char* kLocationsNoDataJson = R"({"owner": "IPMA"})";
static const char* kLocationsMalformed  = R"({not valid json)";

static const char* kForecastJson = R"({
    "owner": "IPMA",
    "country": "PT",
    "data": [
        {
            "forecastDate": "2026-04-02",
            "tMin": "11.8",
            "tMax": "23.9",
            "precipitaProb": "0.0",
            "predWindDir": "N",
            "idWeatherType": 2,
            "classWindSpeed": 2
        },
        {
            "forecastDate": "2026-04-03",
            "tMin": null,
            "tMax": null,
            "precipitaProb": "35.0",
            "predWindDir": "NW",
            "idWeatherType": 6,
            "classWindSpeed": 3
        }
    ]
})";

static const char* kWarningsJson = R"([
    {"awarenessLevelID": "green",  "awarenessTypeName": "Vento",          "startTime": "2026-04-02T00:00:00", "endTime": "2026-04-03T00:00:00", "idAreaAviso": "AVR"},
    {"awarenessLevelID": "yellow", "awarenessTypeName": "Precipitação",   "startTime": "2026-04-02T06:00:00", "endTime": "2026-04-03T06:00:00", "idAreaAviso": "LIS"},
    {"awarenessLevelID": "orange", "awarenessTypeName": "Agitação Marítima", "startTime": "2026-04-02T12:00:00", "endTime": "2026-04-04T00:00:00", "idAreaAviso": "FAR"},
    {"awarenessLevelID": "green",  "awarenessTypeName": "Neve",           "startTime": "2026-04-02T00:00:00", "endTime": "2026-04-05T00:00:00", "idAreaAviso": "AVR"}
])";

static const char* kWeatherTypesJson = R"([
    {"idWeatherType": 1, "descIdWeatherTypePT": "Céu limpo",          "descIdWeatherTypeEN": "Clear sky"},
    {"idWeatherType": 2, "descIdWeatherTypePT": "Poucas nuvens",      "descIdWeatherTypeEN": "Partly cloudy"},
    {"idWeatherType": 6, "descIdWeatherTypePT": "Aguaceiros",         "descIdWeatherTypeEN": "Showers"}
])";

// ── parseLocations ────────────────────────────────────────────────────────────

TEST_CASE("parseLocations: parses data array from wrapped JSON")
{
    const auto locs = IpmaClient::parseLocations(kLocationsJson);
    REQUIRE(locs.size() == 3);

    CHECK(locs[0].globalIdLocal == 1110600);
    CHECK(locs[0].local         == "Lisboa");
    CHECK(locs[0].idRegiao      == "1");
    CHECK(locs[0].idAreaAviso   == "LIS");

    CHECK(locs[1].local == "Porto");
    CHECK(locs[2].local == "Funchal");
    CHECK(locs[2].idRegiao == "2");
}

TEST_CASE("parseLocations: empty when 'data' key missing")
{
    const auto locs = IpmaClient::parseLocations(kLocationsNoDataJson);
    CHECK(locs.empty());
}

TEST_CASE("parseLocations: empty on malformed JSON")
{
    const auto locs = IpmaClient::parseLocations(kLocationsMalformed);
    CHECK(locs.empty());
}

TEST_CASE("parseLocations: empty on empty string")
{
    const auto locs = IpmaClient::parseLocations("");
    CHECK(locs.empty());
}

// ── parseForecast ─────────────────────────────────────────────────────────────

TEST_CASE("parseForecast: parses numeric-string temperature fields")
{
    const auto f = IpmaClient::parseForecast(kForecastJson, 1110600, "Lisboa");
    REQUIRE(f.days.size() == 2);

    CHECK(f.globalIdLocal == 1110600);
    CHECK(f.localName     == "Lisboa");

    const auto& d0 = f.days[0];
    CHECK(d0.forecastDate   == "2026-04-02");
    CHECK(d0.tMin           == 11);
    CHECK(d0.tMax           == 23);
    CHECK(d0.precipitaProb  == 0);
    CHECK(d0.predWindDir    == "N");
    CHECK(d0.idWeatherType  == 2);
    CHECK(d0.classWindSpeed == 2);
}

TEST_CASE("parseForecast: null tMin/tMax yields zero")
{
    const auto f = IpmaClient::parseForecast(kForecastJson, 1110600, "Lisboa");
    REQUIRE(f.days.size() == 2);

    const auto& d1 = f.days[1];
    CHECK(d1.tMin          == 0);
    CHECK(d1.tMax          == 0);
    CHECK(d1.precipitaProb == 35);
    CHECK(d1.idWeatherType == 6);
}

TEST_CASE("parseForecast: empty on malformed JSON")
{
    const auto f = IpmaClient::parseForecast("{bad}", 0, "");
    CHECK(f.days.empty());
}

TEST_CASE("parseForecast: empty days when 'data' key missing")
{
    const auto f = IpmaClient::parseForecast(R"({"owner":"IPMA"})", 0, "");
    CHECK(f.days.empty());
}

// ── parseWarnings ─────────────────────────────────────────────────────────────

TEST_CASE("parseWarnings: filters out green entries")
{
    const auto warns = IpmaClient::parseWarnings(kWarningsJson);
    // Input has 4 entries; 2 are green and should be dropped.
    REQUIRE(warns.size() == 2);
}

TEST_CASE("parseWarnings: uses awarenessTypeName as description")
{
    const auto warns = IpmaClient::parseWarnings(kWarningsJson);
    REQUIRE(warns.size() == 2);

    CHECK(warns[0].awarenessLevelID  == "yellow");
    CHECK(warns[0].awarenessTypeName == "Precipitação");
    CHECK(warns[0].idAreaAviso       == "LIS");
    CHECK(warns[0].startTime         == "2026-04-02T06:00:00");
    CHECK(warns[0].endTime           == "2026-04-03T06:00:00");

    CHECK(warns[1].awarenessLevelID  == "orange");
    CHECK(warns[1].awarenessTypeName == "Agitação Marítima");
    CHECK(warns[1].idAreaAviso       == "FAR");
}

TEST_CASE("parseWarnings: empty on all-green input")
{
    const auto warns = IpmaClient::parseWarnings(
        R"([{"awarenessLevelID":"green","awarenessTypeName":"Vento","startTime":"","endTime":"","idAreaAviso":"X"}])");
    CHECK(warns.empty());
}

TEST_CASE("parseWarnings: empty on malformed JSON")
{
    CHECK(IpmaClient::parseWarnings("").empty());
    CHECK(IpmaClient::parseWarnings("{bad}").empty());
}

// ── parseWeatherTypes ─────────────────────────────────────────────────────────

TEST_CASE("parseWeatherTypes: parses id and descriptions")
{
    const auto types = IpmaClient::parseWeatherTypes(kWeatherTypesJson);
    REQUIRE(types.size() == 3);

    CHECK(types[0].id     == 1);
    CHECK(types[0].descEN == "Clear sky");
    CHECK(types[0].descPT == "Céu limpo");

    CHECK(types[1].id     == 2);
    CHECK(types[1].descEN == "Partly cloudy");

    CHECK(types[2].id     == 6);
    CHECK(types[2].descEN == "Showers");
}

TEST_CASE("parseWeatherTypes: empty on malformed JSON")
{
    CHECK(IpmaClient::parseWeatherTypes("").empty());
}
