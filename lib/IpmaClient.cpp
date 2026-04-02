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
        for (const auto& item : j.value("data", json::array())) {
            IpmaLocation loc;
            loc.globalIdLocal = item.value("globalIdLocal", 0);
            loc.local         = item.value("local", "");
            loc.idRegiao      = std::to_string(item.value("idRegiao", 0));
            loc.idAreaAviso   = item.value("idAreaAviso", "");
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
