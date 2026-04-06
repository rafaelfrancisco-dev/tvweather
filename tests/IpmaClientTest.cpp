#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "MiniTest.h"

#include "ipma/Client.h"
#include <cmath>

// ── Fixture JSON strings (captured from real API responses) ──────────────────

static const char* kLocationsJson = R"({
    "owner": "IPMA",
    "country": "PT",
    "data": [
        {"globalIdLocal": 1110600, "local": "Lisboa",  "idRegiao": 1, "idAreaAviso": "LIS", "latitude": "38.7167", "longitude": "-9.1333"},
        {"globalIdLocal": 1131200, "local": "Porto",   "idRegiao": 1, "idAreaAviso": "PRT", "latitude": "41.1496", "longitude": "-8.6109"},
        {"globalIdLocal": 2310300, "local": "Funchal", "idRegiao": 2, "idAreaAviso": "MAD", "latitude": "32.6669", "longitude": "-16.9241"}
    ]
})";

static const char* kLocationsNoDataJson = R"({"owner": "IPMA"})";
static const char* kLocationsMalformed  = R"({not valid json)";
static const char* kLocationsArrayJson = R"([
    {"globalIdLocal": 1110600, "local": "Lisboa", "idRegiao": 1, "idAreaAviso": "LIS", "latitude": "38.7167", "longitude": "-9.1333"},
    {"globalIdLocal": 1110922, "local": "Praia da Foz do Lizandro", "idRegiao": 1, "idAreaAviso": "LIS", "latitude": "38.9500", "longitude": "-9.4200"}
])";

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

static const char* kWeatherTypesObjectJson = R"({
    "-99": {"PT": "---", "EN": "---"},
    "2": {"PT": "Céu pouco nublado", "EN": "Partly cloudy"},
    "1": {"PT": "Céu limpo", "EN": "Clear sky"}
})";

static const char* kWindTypesJson = R"({
    "1": {"PT": "Fraco", "EN": "Weak"},
    "2": {"PT": "Moderado", "EN": "Moderate"},
    "3": {"PT": "Forte", "EN": "Strong"}
})";

static const char* kWindDirectionTypesJson = R"({
    "N": {"PT": "Norte", "EN": "North", "SiglaPT": "N", "SiglaEN": "N", "DescPT": "Vento de norte", "DescEN": "Wind from North"},
    "NW": {"PT": "Noroeste", "EN": "Northwest", "SiglaPT": "NO", "SiglaEN": "NW", "DescPT": "Vento de noroeste", "DescEN": "Wind from Northwest"}
})";

static const char* kAggregateForecastJson = R"([
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-02T00:00:00",
        "idPeriodo": 24,
        "tMin": "11.8",
        "tMax": "23.9",
        "probabilidadePrecipita": "0.0",
        "idTipoTempo": 2,
        "idFfxVento": 2,
        "iUv": "6.1",
        "intervaloHora": "13h-14h",
        "ddVento": "N",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-02T15:00:00",
        "idPeriodo": 1,
        "tMed": "16.6",
        "utci": "13.8",
        "probabilidadePrecipita": "0.0",
        "idTipoTempo": 1,
        "hR": "71.3",
        "ffVento": "31.3",
        "ddVento": "N",
        "marTotal": "2.3",
        "dirOndulacao": "N",
        "periodOndulacao": "5.6",
        "periodoPico": "9.5",
        "tempAguaMar": "14.4",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-02T12:00:00",
        "idPeriodo": 1,
        "tMed": "16.3",
        "utci": "14.2",
        "probabilidadePrecipita": "0.0",
        "idTipoTempo": 1,
        "hR": "79.5",
        "ffVento": "23.8",
        "ddVento": "N",
        "marTotal": "2.1",
        "dirOndulacao": "N",
        "periodOndulacao": "5.7",
        "periodoPico": "10.1",
        "tempAguaMar": "14.4",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-03T00:00:00",
        "idPeriodo": 24,
        "tMin": "11.3",
        "tMax": "20.2",
        "probabilidadePrecipita": "35.0",
        "idTipoTempo": 6,
        "idFfxVento": 3,
        "iUv": "6.2",
        "intervaloHora": "13h-14h",
        "ddVento": "NW",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-03T02:00:00",
        "idPeriodo": 1,
        "tMed": "12.5",
        "utci": "11.2",
        "probabilidadePrecipita": "-99.0",
        "idTipoTempo": 1,
        "hR": "91.7",
        "ffVento": "12.6",
        "ddVento": "N",
        "marTotal": "1.9",
        "dirOndulacao": "N",
        "periodOndulacao": "5.4",
        "periodoPico": "9.6",
        "tempAguaMar": "14.2",
        "dataUpdate": "2026-04-02T07:53:40"
    }
])";

static const char* kAggregateForecastWithUnavailableDaysJson = R"([
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-02T00:00:00",
        "idPeriodo": 24,
        "tMin": "11.8",
        "tMax": "23.9",
        "probabilidadePrecipita": "0.0",
        "idTipoTempo": 2,
        "idFfxVento": 2,
        "iUv": "6.1",
        "intervaloHora": "13h-14h",
        "ddVento": "N",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-02T09:00:00",
        "idPeriodo": 1,
        "tMed": "17.5",
        "utci": "16.8",
        "probabilidadePrecipita": "5.0",
        "idTipoTempo": 2,
        "hR": "60.0",
        "ffVento": "8.0",
        "ddVento": "N",
        "marTotal": "-99.0",
        "dirOndulacao": "",
        "periodOndulacao": "-99.0",
        "periodoPico": "-99.0",
        "tempAguaMar": "-99.0",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-03T00:00:00",
        "idPeriodo": 24,
        "tMin": "-99.0",
        "tMax": "-99.0",
        "probabilidadePrecipita": "-99.0",
        "idTipoTempo": -99,
        "idFfxVento": "-99.0",
        "iUv": "-99.0",
        "intervaloHora": "",
        "ddVento": "",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-03T03:00:00",
        "idPeriodo": 1,
        "tMed": "-99.0",
        "utci": "-99.0",
        "probabilidadePrecipita": "-99.0",
        "idTipoTempo": -99,
        "hR": "-99.0",
        "ffVento": "-99.0",
        "ddVento": "",
        "marTotal": "-99.0",
        "dirOndulacao": "",
        "periodOndulacao": "-99.0",
        "periodoPico": "-99.0",
        "tempAguaMar": "-99.0",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-04T00:00:00",
        "idPeriodo": 24,
        "tMin": "10.0",
        "tMax": "19.0",
        "probabilidadePrecipita": "20.0",
        "idTipoTempo": 1,
        "idFfxVento": 1,
        "iUv": "5.0",
        "intervaloHora": "12h-13h",
        "ddVento": "N",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-04T03:00:00",
        "idPeriodo": 1,
        "tMed": "-99.0",
        "utci": "-99.0",
        "probabilidadePrecipita": "-99.0",
        "idTipoTempo": -99,
        "hR": "-99.0",
        "ffVento": "-99.0",
        "ddVento": "",
        "marTotal": "-99.0",
        "dirOndulacao": "",
        "periodOndulacao": "-99.0",
        "periodoPico": "-99.0",
        "tempAguaMar": "-99.0",
        "dataUpdate": "2026-04-02T07:53:40"
    },
    {
        "globalIdLocal": 1110600,
        "dataPrev": "2026-04-04T06:00:00",
        "idPeriodo": 1,
        "tMed": "13.2",
        "utci": "12.4",
        "probabilidadePrecipita": "10.0",
        "idTipoTempo": 1,
        "hR": "82.0",
        "ffVento": "11.0",
        "ddVento": "N",
        "marTotal": "1.3",
        "dirOndulacao": "N",
        "periodOndulacao": "4.5",
        "periodoPico": "8.2",
        "tempAguaMar": "14.1",
        "dataUpdate": "2026-04-02T07:53:40"
    }
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
    REQUIRE(locs[0].latitude.has_value());
    REQUIRE(locs[0].longitude.has_value());
    CHECK(std::abs(*locs[0].latitude - 38.7167) < 0.0001);
    CHECK(std::abs(*locs[0].longitude + 9.1333) < 0.0001);

    CHECK(locs[1].local == "Porto");
    CHECK(locs[2].local == "Funchal");
    CHECK(locs[2].idRegiao == "2");
}

TEST_CASE("parseLocations: empty when 'data' key missing")
{
    const auto locs = IpmaClient::parseLocations(kLocationsNoDataJson);
    CHECK(locs.empty());
}

TEST_CASE("parseLocations: parses top-level location arrays")
{
    const auto locs = IpmaClient::parseLocations(kLocationsArrayJson);
    REQUIRE(locs.size() == 2);
    CHECK(locs[1].globalIdLocal == 1110922);
    CHECK(locs[1].local == "Praia da Foz do Lizandro");
    CHECK(locs[1].idAreaAviso == "LIS");
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

TEST_CASE("parseWeatherTypes: parses object lookup payloads")
{
    const auto types = IpmaClient::parseWeatherTypes(kWeatherTypesObjectJson);
    REQUIRE(types.size() == 3);
    CHECK(types[0].id == -99);
    CHECK(types[1].id == 1);
    CHECK(types[2].descEN == "Partly cloudy");
}

TEST_CASE("parseWindTypes: parses wind strength lookups")
{
    const auto types = IpmaClient::parseWindTypes(kWindTypesJson);
    REQUIRE(types.size() == 3);
    CHECK(types[1].id == 2);
    CHECK(types[1].descPT == "Moderado");
    CHECK(types[2].descEN == "Strong");
}

TEST_CASE("parseWindDirectionTypes: parses direction labels and siglas")
{
    const auto dirs = IpmaClient::parseWindDirectionTypes(kWindDirectionTypesJson);
    REQUIRE(dirs.size() == 2);
    CHECK(dirs[0].id == "N");
    CHECK(dirs[0].descEN == "Wind from North");
    CHECK(dirs[1].siglaPT == "NO");
}

TEST_CASE("parseAggregateForecast: splits daily and hourly rows by day")
{
    const auto forecast = IpmaClient::parseAggregateForecast(kAggregateForecastJson, 1110600, "Lisboa");
    REQUIRE(forecast.days.size() == 2);

    CHECK(forecast.globalIdLocal == 1110600);
    CHECK(forecast.localName == "Lisboa");
    CHECK(forecast.dataUpdate == "2026-04-02T07:53:40");

    const auto& day0 = forecast.days[0];
    CHECK(day0.forecastDate == "2026-04-02");
    REQUIRE(day0.tMin.has_value());
    REQUIRE(day0.tMax.has_value());
    CHECK(*day0.tMin == 11);
    CHECK(*day0.tMax == 23);
    CHECK(day0.idWeatherType == 2);
    REQUIRE(day0.uvIndex.has_value());
    CHECK(*day0.uvIndex > 6.0f && *day0.uvIndex < 6.2f);
    REQUIRE(day0.hours.size() == 2);
    CHECK(day0.hours[0].forecastDateTime == "2026-04-02T12:00:00");
    CHECK(day0.hours[1].forecastDateTime == "2026-04-02T15:00:00");
}

TEST_CASE("parseAggregateForecast: treats -99 sentinels as missing optional values")
{
    const auto forecast = IpmaClient::parseAggregateForecast(kAggregateForecastJson, 1110600, "Lisboa");
    REQUIRE(forecast.days.size() == 2);
    REQUIRE(forecast.days[1].hours.size() == 1);

    const auto& hour = forecast.days[1].hours[0];
    CHECK(!hour.precipProbability.has_value());
    REQUIRE(hour.waterTemperature.has_value());
    CHECK(*hour.waterTemperature > 14.1f && *hour.waterTemperature < 14.3f);
}

TEST_CASE("parseAggregateForecast: filters out days without usable hourly data")
{
    const auto forecast = IpmaClient::parseAggregateForecast(
        kAggregateForecastWithUnavailableDaysJson, 1110600, "Lisboa");

    REQUIRE(forecast.days.size() == 2);
    CHECK(forecast.days[0].forecastDate == "2026-04-02");
    CHECK(forecast.days[1].forecastDate == "2026-04-04");
    REQUIRE(forecast.days[0].hours.empty() == false);
    REQUIRE(forecast.days[1].hours.size() == 1);
    CHECK(forecast.days[1].hours[0].forecastDateTime == "2026-04-04T06:00:00");
}

// ── parseUv ───────────────────────────────────────────────────────────────────

static const char* kUvJson = R"([
    {"globalIdLocal": 1110600, "iUv": 7.2, "intervaloHora": "13h-15h", "data": "2026-04-02", "idPeriodo": 5},
    {"globalIdLocal": 1131200, "iUv": 6.5, "intervaloHora": "12h-14h", "data": "2026-04-02", "idPeriodo": 4},
    {"globalIdLocal": 1110600, "iUv": 5.1, "intervaloHora": "13h-15h", "data": "2026-04-03", "idPeriodo": 5}
])";

TEST_CASE("parseUv: parses UV entries from flat array")
{
    const auto entries = IpmaClient::parseUv(kUvJson);
    REQUIRE(entries.size() == 3);

    CHECK(entries[0].globalIdLocal == 1110600);
    CHECK(entries[0].iUv           > 7.1f && entries[0].iUv < 7.3f);
    CHECK(entries[0].intervaloHora == "13h-15h");
    CHECK(entries[0].data          == "2026-04-02");
    CHECK(entries[0].idPeriodo     == 5);

    CHECK(entries[1].globalIdLocal == 1131200);
    CHECK(entries[2].data          == "2026-04-03");
}

TEST_CASE("parseUv: empty on malformed JSON")
{
    CHECK(IpmaClient::parseUv("").empty());
    CHECK(IpmaClient::parseUv("{bad}").empty());
}

// ── parseSeaLocations ─────────────────────────────────────────────────────────

static const char* kSeaLocationsJson = R"([
    {"globalIdLocal": 1130826, "local": "Porto, costa", "latitude": "41.175", "longitude": "-8.76"},
    {"globalIdLocal": 1111026, "local": "Lisboa, costa", "latitude": "38.65", "longitude": "-9.31"},
    {"globalIdLocal": 1080526, "local": "Faro, costa", "latitude": "37.0017", "longitude": "-8.0"}
])";

static const char* kSeaLocationsWrappedJson = R"({
    "owner": "IPMA",
    "data": [
        {"globalIdLocal": 3420100, "local": "Porto"},
        {"globalIdLocal": 3480200, "local": "Lisboa"},
        {"globalIdLocal": 3540100, "local": "Faro"}
    ]
})";

TEST_CASE("parseSeaLocations: parses coastal locations")
{
    const auto locs = IpmaClient::parseSeaLocations(kSeaLocationsJson);
    REQUIRE(locs.size() == 3);

    CHECK(locs[0].globalIdLocal == 1130826);
    CHECK(locs[0].local         == "Porto, costa");
    REQUIRE(locs[0].latitude.has_value());
    REQUIRE(locs[0].longitude.has_value());
    CHECK(std::abs(*locs[0].latitude - 41.175) < 0.0001);
    CHECK(std::abs(*locs[0].longitude + 8.76) < 0.0001);
    CHECK(locs[1].local         == "Lisboa, costa");
    CHECK(locs[2].globalIdLocal == 1080526);
}

TEST_CASE("parseSeaLocations: supports wrapped legacy payload")
{
    const auto locs = IpmaClient::parseSeaLocations(kSeaLocationsWrappedJson);
    REQUIRE(locs.size() == 3);

    CHECK(locs[0].globalIdLocal == 3420100);
    CHECK(locs[1].local         == "Lisboa");
}

TEST_CASE("parseSeaLocations: empty when 'data' key missing")
{
    CHECK(IpmaClient::parseSeaLocations(R"({"owner":"IPMA"})").empty());
}

TEST_CASE("parseSeaLocations: empty on malformed JSON")
{
    CHECK(IpmaClient::parseSeaLocations("").empty());
}

// ── parseSeaForecastDay ───────────────────────────────────────────────────────

static const char* kSeaDayJson = R"({
    "owner": "IPMA",
    "forecastDate": "2026-04-02",
    "data": [
        {
            "globalIdLocal": 1111026,
            "waveHighMin": "0.9",
            "waveHighMax": "1.3",
            "wavePeriodMin": "4.9",
            "wavePeriodMax": "5.8",
            "predWaveDir": "NW",
            "sstMin": "14.9",
            "sstMax": "15.1"
        },
        {
            "globalIdLocal": 1130826,
            "waveHighMin": "0.8",
            "waveHighMax": "1.6",
            "wavePeriodMin": "4.8",
            "wavePeriodMax": "6.1",
            "predWaveDir": "NW",
            "sstMin": "13.2",
            "sstMax": "13.4"
        }
    ]
})";

static const char* kSeaDayLegacyJson = R"([
    {
        "globalIdLocal": 3480200,
        "dataPrev": "2026-04-02",
        "waveHeightMin": 0.5,
        "waveHeightMax": 1.2,
        "wavePeriodMin": 8.0,
        "wavePeriodMax": 11.0,
        "predWaveDir": "NW",
        "sstMin": 16.2,
        "sstMax": 17.1
    }
])";

TEST_CASE("parseSeaForecastDay: returns only the entry matching globalIdLocal")
{
    const auto days = IpmaClient::parseSeaForecastDay(kSeaDayJson, 1111026);
    REQUIRE(days.size() == 1);

    CHECK(days[0].forecastDate  == "2026-04-02");
    CHECK(days[0].waveHeightMin > 0.8f && days[0].waveHeightMin < 1.0f);
    CHECK(days[0].waveHeightMax > 1.2f && days[0].waveHeightMax < 1.4f);
    CHECK(days[0].wavePeriodMin > 4.8f && days[0].wavePeriodMin < 5.0f);
    CHECK(days[0].wavePeriodMax > 5.7f && days[0].wavePeriodMax < 5.9f);
    CHECK(days[0].predWaveDir   == "NW");
    CHECK(days[0].sstMin        > 14.8f && days[0].sstMin < 15.0f);
    CHECK(days[0].sstMax        > 15.0f && days[0].sstMax < 15.2f);
}

TEST_CASE("parseSeaForecastDay: supports legacy array payload")
{
    const auto days = IpmaClient::parseSeaForecastDay(kSeaDayLegacyJson, 3480200);
    REQUIRE(days.size() == 1);

    CHECK(days[0].forecastDate  == "2026-04-02");
    CHECK(days[0].waveHeightMin > 0.4f && days[0].waveHeightMin < 0.6f);
    CHECK(days[0].waveHeightMax > 1.1f && days[0].waveHeightMax < 1.3f);
}

TEST_CASE("parseSeaForecastDay: empty when globalIdLocal not found")
{
    const auto days = IpmaClient::parseSeaForecastDay(kSeaDayJson, 9999999);
    CHECK(days.empty());
}

TEST_CASE("parseSeaForecastDay: empty on malformed JSON")
{
    CHECK(IpmaClient::parseSeaForecastDay("", 3480200).empty());
}
