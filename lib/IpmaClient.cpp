#include "ipma/Client.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#ifdef _WIN32
#  define POPEN  _popen
#  define PCLOSE _pclose
#else
#  define POPEN  popen
#  define PCLOSE pclose
#endif

using json = nlohmann::json;

namespace {

std::optional<double> parseNumber(const json& value, bool treatMinus99AsMissing = false)
{
    double parsed = 0.0;

    if (value.is_number())
        parsed = value.get<double>();
    else if (value.is_string()) {
        try {
            parsed = std::stod(value.get<std::string>());
        } catch (...) {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    if (treatMinus99AsMissing && parsed <= -99.0)
        return std::nullopt;

    return parsed;
}

std::optional<double> parseNumber(const json& obj,
                                  const char* key,
                                  bool treatMinus99AsMissing = false)
{
    const auto it = obj.find(key);
    if (it == obj.end() || it->is_null())
        return std::nullopt;
    return parseNumber(*it, treatMinus99AsMissing);
}

std::optional<int> parseInt(const json& obj,
                            const char* key,
                            bool treatMinus99AsMissing = false)
{
    if (const auto parsed = parseNumber(obj, key, treatMinus99AsMissing))
        return static_cast<int>(*parsed);
    return std::nullopt;
}

std::optional<float> parseFloat(const json& obj,
                                const char* key,
                                bool treatMinus99AsMissing = false)
{
    if (const auto parsed = parseNumber(obj, key, treatMinus99AsMissing))
        return static_cast<float>(*parsed);
    return std::nullopt;
}

std::optional<double> parseCoordinate(const json& obj, const char* key)
{
    return parseNumber(obj, key, false);
}

const json* extractItems(const json& root)
{
    if (root.is_array())
        return &root;
    if (root.is_object()) {
        const auto it = root.find("data");
        if (it != root.end() && it->is_array())
            return &(*it);
    }
    return nullptr;
}

std::string forecastDatePart(const std::string& dateTime)
{
    return dateTime.size() >= 10 ? dateTime.substr(0, 10) : dateTime;
}

bool hasAggregateHourData(const IpmaAggregateHour& hour)
{
    return hour.temperature.has_value()
        || hour.feltTemperature.has_value()
        || hour.precipProbability.has_value()
        || hour.idWeatherType != -99
        || hour.humidity.has_value()
        || hour.windSpeed.has_value()
        || !hour.windDirection.empty()
        || hour.seaTotal.has_value()
        || !hour.waveDirection.empty()
        || hour.wavePeriod.has_value()
        || hour.peakPeriod.has_value()
        || hour.waterTemperature.has_value();
}

} // namespace

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
        const auto j = json::parse(raw);
        const json* items = extractItems(j);
        if (items == nullptr)
            return out;

        for (const auto& item : *items) {
            IpmaLocation loc;
            loc.globalIdLocal = item.value("globalIdLocal", 0);
            loc.local         = item.value("local", "");
            loc.idRegiao      = std::to_string(item.value("idRegiao", 0));
            loc.idAreaAviso   = item.value("idAreaAviso", "");
            loc.latitude      = parseCoordinate(item, "latitude");
            loc.longitude     = parseCoordinate(item, "longitude");
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
        const auto j = json::parse(raw);
        const json* items = extractItems(j);
        if (items == nullptr)
            return forecast;

        for (const auto& d : *items) {
            IpmaForecastDay day;
            day.forecastDate   = d.value("forecastDate",  "");
            day.tMin           = parseInt(d, "tMin").value_or(0);
            day.tMax           = parseInt(d, "tMax").value_or(0);
            day.precipitaProb  = parseInt(d, "precipitaProb").value_or(0);
            day.idWeatherType  = parseInt(d, "idWeatherType").value_or(0);
            day.classWindSpeed = parseInt(d, "classWindSpeed").value_or(0);
            day.predWindDir    = d.value("predWindDir", "");
            forecast.days.push_back(std::move(day));
        }
    } catch (...) {}
    return forecast;
}

IpmaAggregateForecast IpmaClient::parseAggregateForecast(const std::string& raw,
                                                         int globalIdLocal,
                                                         const std::string& localName)
{
    IpmaAggregateForecast forecast;
    forecast.globalIdLocal = globalIdLocal;
    forecast.localName     = localName;

    try {
        const auto j = json::parse(raw);
        if (!j.is_array())
            return forecast;

        auto ensureDay = [&forecast](const std::string& forecastDate) -> IpmaAggregateDay& {
            const auto it = std::find_if(forecast.days.begin(),
                                         forecast.days.end(),
                                         [&forecastDate](const IpmaAggregateDay& day) {
                                             return day.forecastDate == forecastDate;
                                         });
            if (it != forecast.days.end())
                return *it;

            forecast.days.push_back(IpmaAggregateDay{});
            forecast.days.back().forecastDate = forecastDate;
            return forecast.days.back();
        };

        for (const auto& item : j) {
            const int itemGlobalIdLocal = item.value("globalIdLocal", globalIdLocal);
            if (itemGlobalIdLocal != globalIdLocal)
                continue;

            const std::string dateTime = item.value("dataPrev", "");
            const std::string dayDate = forecastDatePart(dateTime);
            if (dayDate.empty())
                continue;

            IpmaAggregateDay& day = ensureDay(dayDate);
            const std::string itemUpdate = item.value("dataUpdate", "");
            if (forecast.dataUpdate.empty() && !itemUpdate.empty())
                forecast.dataUpdate = itemUpdate;
            if (day.dataUpdate.empty() && !itemUpdate.empty())
                day.dataUpdate = itemUpdate;

            const int periodId = item.value("idPeriodo", 0);
            if (periodId == 24) {
                day.tMin                = parseInt(item, "tMin", true);
                day.tMax                = parseInt(item, "tMax", true);
                day.precipProbability   = parseInt(item, "probabilidadePrecipita", true);
                day.uvIndex             = parseFloat(item, "iUv", true);
                day.uvInterval          = item.value("intervaloHora", "");
                day.idWeatherType       = parseInt(item, "idTipoTempo", true).value_or(-99);
                day.windClass           = parseInt(item, "idFfxVento", true);
                day.windDirection       = item.value("ddVento", "");
            } else if (periodId == 1) {
                IpmaAggregateHour hour;
                hour.forecastDateTime   = dateTime;
                hour.forecastDate       = dayDate;
                hour.temperature        = parseFloat(item, "tMed", true);
                hour.feltTemperature    = parseFloat(item, "utci", true);
                hour.precipProbability  = parseInt(item, "probabilidadePrecipita", true);
                hour.idWeatherType      = parseInt(item, "idTipoTempo", true).value_or(-99);
                hour.humidity           = parseFloat(item, "hR", true);
                hour.windSpeed          = parseFloat(item, "ffVento", true);
                hour.windDirection      = item.value("ddVento", "");
                hour.seaTotal           = parseFloat(item, "marTotal", true);
                hour.waveDirection      = item.value("dirOndulacao", "");
                hour.wavePeriod         = parseFloat(item, "periodOndulacao", true);
                hour.peakPeriod         = parseFloat(item, "periodoPico", true);
                hour.waterTemperature   = parseFloat(item, "tempAguaMar", true);
                hour.dataUpdate         = itemUpdate;
                day.hours.push_back(std::move(hour));
            }
        }

        std::sort(forecast.days.begin(),
                  forecast.days.end(),
                  [](const IpmaAggregateDay& left, const IpmaAggregateDay& right) {
                      return left.forecastDate < right.forecastDate;
                  });

        for (auto& day : forecast.days) {
            day.hours.erase(std::remove_if(day.hours.begin(),
                                           day.hours.end(),
                                           [](const IpmaAggregateHour& hour) {
                                               return !hasAggregateHourData(hour);
                                           }),
                            day.hours.end());
            std::sort(day.hours.begin(),
                      day.hours.end(),
                      [](const IpmaAggregateHour& left, const IpmaAggregateHour& right) {
                          return left.forecastDateTime < right.forecastDateTime;
                      });
        }

        forecast.days.erase(std::remove_if(forecast.days.begin(),
                                           forecast.days.end(),
                                           [](const IpmaAggregateDay& day) {
                                               return day.hours.empty();
                                           }),
                            forecast.days.end());
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
        if (j.is_array()) {
            for (const auto& t : j) {
                IpmaWeatherType wt;
                wt.id     = t.value("idWeatherType", 0);
                wt.descPT = t.value("descIdWeatherTypePT", "");
                wt.descEN = t.value("descIdWeatherTypeEN", "");
                out.push_back(std::move(wt));
            }
        } else if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                IpmaWeatherType wt;
                wt.id     = std::stoi(it.key());
                wt.descPT = it.value().value("PT", "");
                wt.descEN = it.value().value("EN", "");
                out.push_back(std::move(wt));
            }
            std::sort(out.begin(), out.end(),
                      [](const IpmaWeatherType& left, const IpmaWeatherType& right) {
                          return left.id < right.id;
                      });
        }
    } catch (...) {}
    return out;
}

std::vector<IpmaWindType> IpmaClient::parseWindTypes(const std::string& raw)
{
    std::vector<IpmaWindType> out;
    try {
        const auto j = json::parse(raw);
        if (!j.is_object())
            return out;

        for (auto it = j.begin(); it != j.end(); ++it) {
            IpmaWindType wt;
            wt.id     = std::stoi(it.key());
            wt.descPT = it.value().value("PT", "");
            wt.descEN = it.value().value("EN", "");
            out.push_back(std::move(wt));
        }

        std::sort(out.begin(), out.end(),
                  [](const IpmaWindType& left, const IpmaWindType& right) {
                      return left.id < right.id;
                  });
    } catch (...) {}
    return out;
}

std::vector<IpmaWindDirectionType> IpmaClient::parseWindDirectionTypes(
    const std::string& raw)
{
    std::vector<IpmaWindDirectionType> out;
    try {
        const auto j = json::parse(raw);
        if (!j.is_object())
            return out;

        for (auto it = j.begin(); it != j.end(); ++it) {
            IpmaWindDirectionType dir;
            dir.id      = it.key();
            dir.labelPT = it.value().value("PT", "");
            dir.labelEN = it.value().value("EN", "");
            dir.siglaPT = it.value().value("SiglaPT", "");
            dir.siglaEN = it.value().value("SiglaEN", "");
            dir.descPT  = it.value().value("DescPT", "");
            dir.descEN  = it.value().value("DescEN", "");
            out.push_back(std::move(dir));
        }

        std::sort(out.begin(), out.end(),
                  [](const IpmaWindDirectionType& left, const IpmaWindDirectionType& right) {
                      return left.id < right.id;
                  });
    } catch (...) {}
    return out;
}

// ── Fetch = httpGet + parse ───────────────────────────────────────────────────

std::vector<IpmaLocation> IpmaClient::fetchLocations()
{
    auto locations = parseLocations(
        httpGet(std::string(kBase) + "/public-data/forecast/locations.json"));
    if (!locations.empty())
        return locations;

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

IpmaAggregateForecast IpmaClient::fetchAggregateForecast(int globalIdLocal,
                                                         const std::string& localName)
{
    return parseAggregateForecast(
        httpGet(std::string(kBase)
            + "/public-data/forecast/aggregate/"
            + std::to_string(globalIdLocal) + ".json"),
        globalIdLocal,
        localName);
}

std::vector<IpmaWarning> IpmaClient::fetchWarnings()
{
    auto warnings = parseWarnings(
        httpGet(std::string(kBase)
            + "/public-data/warnings/warnings_www.json"));
    if (!warnings.empty())
        return warnings;

    return parseWarnings(
        httpGet(std::string(kBase)
            + "/open-data/forecast/warnings/warnings_www.json"));
}

std::vector<IpmaWeatherType> IpmaClient::fetchWeatherTypes()
{
    auto types = parseWeatherTypes(
        httpGet("https://www.ipma.pt/opencms/bin/file.data/weathertypes.json"));
    if (!types.empty())
        return types;

    return parseWeatherTypes(
        httpGet(std::string(kBase)
            + "/open-data/weather-type-classe.json"));
}

std::vector<IpmaWindType> IpmaClient::fetchWindTypes()
{
    return parseWindTypes(
        httpGet("https://www.ipma.pt/opencms/bin/file.data/windtypes2023.json"));
}

std::vector<IpmaWindDirectionType> IpmaClient::fetchWindDirectionTypes()
{
    return parseWindDirectionTypes(
        httpGet("https://www.ipma.pt/opencms/bin/file.data/winddirtypes2023.json"));
}

std::vector<IpmaUvEntry> IpmaClient::parseUv(const std::string& raw)
{
    std::vector<IpmaUvEntry> out;
    try {
        const auto j = json::parse(raw);
        for (const auto& item : j) {
            IpmaUvEntry e;
            e.globalIdLocal = item.value("globalIdLocal", 0);
            e.iUv           = parseFloat(item, "iUv").value_or(0.0f);
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
        const json* items = extractItems(j);

        if (!items)
            return out;

        for (const auto& item : *items) {
            IpmaSeaLocation loc;
            loc.globalIdLocal = item.value("globalIdLocal", 0);
            loc.local         = item.value("local", "");
            loc.latitude      = parseCoordinate(item, "latitude");
            loc.longitude     = parseCoordinate(item, "longitude");
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
