#include "ipma/Client.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdio>
#include <string>

#ifdef _WIN32
#  define POPEN  _popen
#  define PCLOSE _pclose
#else
#  define POPEN  popen
#  define PCLOSE pclose
#endif

using json = nlohmann::json;

// ── HTTP ──────────────────────────────────────────────────────────────────────

std::string IpmaClient::httpGet(const std::string& url)
{
    std::string cmd = "curl -s --max-time 10 \"" + url + "\"";
    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe)
        return {};
    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe))
        result += buf;
    PCLOSE(pipe);
    return result;
}

// ── Parse ─────────────────────────────────────────────────────────────────────

std::vector<IpmaLocation> IpmaClient::parseLocations(const std::string& raw)
{
    std::vector<IpmaLocation> out;
    try {
        // Root: { "owner": "IPMA", "country": "PT", "data": [...] }
        const auto j = json::parse(raw);
        auto safeDouble = [](const json& obj, const char* key) -> std::optional<double> {
            const auto it = obj.find(key);
            if (it == obj.end() || it->is_null())
                return std::nullopt;
            if (it->is_number())
                return it->get<double>();
            if (it->is_string()) {
                try {
                    return std::stod(it->get<std::string>());
                } catch (...) {}
            }
            return std::nullopt;
        };

        for (const auto& item : j.value("data", json::array())) {
            IpmaLocation loc;
            loc.globalIdLocal = item.value("globalIdLocal", 0);
            loc.local         = item.value("local", "");
            loc.idRegiao      = std::to_string(item.value("idRegiao", 0));
            loc.idAreaAviso   = item.value("idAreaAviso", "");
            loc.latitude      = safeDouble(item, "latitude");
            loc.longitude     = safeDouble(item, "longitude");
            if (!loc.local.empty())
                out.push_back(std::move(loc));
        }
    } catch (...) {}
    return out;
}

IpmaForecast IpmaClient::parseForecast(const std::string& raw,
                                        int    globalIdLocal,
                                        const std::string& localName)
{
    IpmaForecast forecast;
    forecast.globalIdLocal = globalIdLocal;
    forecast.localName     = localName;
    try {
        // Root: { "owner": "IPMA", "country": "PT", "data": [...] }
        const auto j = json::parse(raw);

        // Fields tMin/tMax/precipitaProb come as numeric strings ("11.8").
        auto safeInt = [](const json& obj, const char* key) -> int {
            const auto it = obj.find(key);
            if (it == obj.end() || it->is_null()) return 0;
            if (it->is_number()) return static_cast<int>(it->get<double>());
            if (it->is_string()) {
                try {
                    return static_cast<int>(std::stod(it->get<std::string>()));
                } catch (...) { return 0; }
            }
            return 0;
        };

        for (const auto& d : j.value("data", json::array())) {
            IpmaForecastDay day;
            day.forecastDate   = d.value("forecastDate",  "");
            day.tMin           = safeInt(d, "tMin");
            day.tMax           = safeInt(d, "tMax");
            day.precipitaProb  = safeInt(d, "precipitaProb");
            day.idWeatherType  = safeInt(d, "idWeatherType");
            day.classWindSpeed = safeInt(d, "classWindSpeed");
            day.predWindDir    = d.value("predWindDir", "");
            forecast.days.push_back(std::move(day));
        }
    } catch (...) {}
    return forecast;
}

std::vector<IpmaWarning> IpmaClient::parseWarnings(const std::string& raw)
{
    std::vector<IpmaWarning> out;
    try {
        // Root: flat array (no "data" wrapper)
        const auto j = json::parse(raw);
        for (const auto& w : j) {
            const std::string level = w.value("awarenessLevelID", "");
            // "green" entries are baseline placeholders ("no risk") — skip them.
            if (level == "green") continue;

            IpmaWarning warn;
            warn.awarenessLevelID  = level;
            warn.awarenessTypeName = w.value("awarenessTypeName", "");
            warn.startTime         = w.value("startTime",  "");
            warn.endTime           = w.value("endTime",    "");
            warn.idAreaAviso       = w.value("idAreaAviso", "");
            out.push_back(std::move(warn));
        }
    } catch (...) {}
    return out;
}

std::vector<IpmaWeatherType> IpmaClient::parseWeatherTypes(const std::string& raw)
{
    std::vector<IpmaWeatherType> out;
    try {
        const auto j = json::parse(raw);
        for (const auto& t : j) {
            IpmaWeatherType wt;
            wt.id     = t.value("idWeatherType", 0);
            wt.descPT = t.value("descIdWeatherTypePT", "");
            wt.descEN = t.value("descIdWeatherTypeEN", "");
            out.push_back(std::move(wt));
        }
    } catch (...) {}
    return out;
}

// ── Fetch = httpGet + parse ───────────────────────────────────────────────────

std::vector<IpmaLocation> IpmaClient::fetchLocations()
{
    return parseLocations(
        httpGet(std::string(kBase) + "/open-data/distrits-islands.json"));
}

IpmaForecast IpmaClient::fetchForecast(int globalIdLocal,
                                        const std::string& localName)
{
    return parseForecast(
        httpGet(std::string(kBase)
            + "/open-data/forecast/meteorology/cities/daily/"
            + std::to_string(globalIdLocal) + ".json"),
        globalIdLocal, localName);
}

std::vector<IpmaWarning> IpmaClient::fetchWarnings()
{
    return parseWarnings(
        httpGet(std::string(kBase)
            + "/open-data/forecast/warnings/warnings_www.json"));
}

std::vector<IpmaWeatherType> IpmaClient::fetchWeatherTypes()
{
    return parseWeatherTypes(
        httpGet(std::string(kBase)
            + "/open-data/weather-type-classe.json"));
}

std::vector<IpmaUvEntry> IpmaClient::parseUv(const std::string& raw)
{
    std::vector<IpmaUvEntry> out;
    try {
        const auto j = json::parse(raw);
        for (const auto& item : j) {
            IpmaUvEntry e;
            e.globalIdLocal = item.value("globalIdLocal", 0);
            e.iUv           = item.value("iUv", 0.0f);
            e.intervaloHora = item.value("intervaloHora", "");
            e.data          = item.value("data", "");
            e.idPeriodo     = item.value("idPeriodo", 0);
            if (e.globalIdLocal != 0)
                out.push_back(std::move(e));
        }
    } catch (...) {}
    return out;
}

std::vector<IpmaSeaLocation> IpmaClient::parseSeaLocations(const std::string& raw)
{
    std::vector<IpmaSeaLocation> out;
    try {
        const auto j = json::parse(raw);
        auto safeDouble = [](const json& obj, const char* key) -> std::optional<double> {
            const auto it = obj.find(key);
            if (it == obj.end() || it->is_null())
                return std::nullopt;
            if (it->is_number())
                return it->get<double>();
            if (it->is_string()) {
                try {
                    return std::stod(it->get<std::string>());
                } catch (...) {}
            }
            return std::nullopt;
        };

        const json* items = nullptr;
        if (j.is_array())
            items = &j;
        else if (const auto it = j.find("data"); it != j.end() && it->is_array())
            items = &(*it);

        if (!items)
            return out;

        for (const auto& item : *items) {
            IpmaSeaLocation loc;
            loc.globalIdLocal = item.value("globalIdLocal", 0);
            loc.local         = item.value("local", "");
            loc.latitude      = safeDouble(item, "latitude");
            loc.longitude     = safeDouble(item, "longitude");
            if (!loc.local.empty())
                out.push_back(std::move(loc));
        }
    } catch (...) {}
    return out;
}

std::vector<IpmaSeaForecastDay> IpmaClient::parseSeaForecastDay(
    const std::string& raw, int globalIdLocal)
{
    std::vector<IpmaSeaForecastDay> out;
    try {
        const auto j = json::parse(raw);
        const json* items = nullptr;
        std::string forecastDate;

        if (j.is_array()) {
            items = &j;
        } else if (j.is_object()) {
            forecastDate = j.value("forecastDate", "");
            if (const auto it = j.find("data"); it != j.end() && it->is_array())
                items = &(*it);
        }

        if (!items)
            return out;

        auto safeFloat = [](const json& obj,
                            std::initializer_list<const char*> keys) -> float {
            for (const char* key : keys) {
                const auto it = obj.find(key);
                if (it == obj.end() || it->is_null())
                    continue;
                if (it->is_number())
                    return static_cast<float>(it->get<double>());
                if (it->is_string()) {
                    try {
                        return std::stof(it->get<std::string>());
                    } catch (...) {}
                }
            }
            return 0.0f;
        };

        for (const auto& item : *items) {
            if (item.value("globalIdLocal", 0) != globalIdLocal)
                continue;

            IpmaSeaForecastDay day;
            day.forecastDate   = item.value("dataPrev",
                                  item.value("forecastDate", forecastDate));
            day.waveHeightMin  = safeFloat(item, {"waveHeightMin", "waveHighMin"});
            day.waveHeightMax  = safeFloat(item, {"waveHeightMax", "waveHighMax"});
            day.wavePeriodMin  = safeFloat(item, {"wavePeriodMin"});
            day.wavePeriodMax  = safeFloat(item, {"wavePeriodMax"});
            day.predWaveDir    = item.value("predWaveDir", "");
            day.sstMin         = safeFloat(item, {"sstMin"});
            day.sstMax         = safeFloat(item, {"sstMax"});
            out.push_back(std::move(day));
            break; // one entry per location per day file
        }
    } catch (...) {}
    return out;
}

std::vector<IpmaUvEntry> IpmaClient::fetchUv()
{
    return parseUv(
        httpGet(std::string(kBase)
            + "/open-data/forecast/meteorology/uv/uv.json"));
}

std::vector<IpmaSeaLocation> IpmaClient::fetchSeaLocations()
{
    return parseSeaLocations(
        httpGet(std::string(kBase) + "/open-data/sea-locations.json"));
}

IpmaSeaForecast IpmaClient::fetchSeaForecast(int globalIdLocal,
                                              const std::string& localName)
{
    IpmaSeaForecast forecast;
    forecast.globalIdLocal = globalIdLocal;
    forecast.localName     = localName;

    for (int day = 0; day <= 2; ++day) {
        const std::string url = std::string(kBase)
            + "/open-data/forecast/oceanography/daily/hp-daily-sea-forecast-day"
            + std::to_string(day) + ".json";
        auto entries = parseSeaForecastDay(httpGet(url), globalIdLocal);
        for (auto& e : entries)
            forecast.days.push_back(std::move(e));
    }
    return forecast;
}
