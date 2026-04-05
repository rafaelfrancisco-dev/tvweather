#pragma once
#include <optional>
#include <string>
#include <vector>

struct IpmaLocation {
    int         globalIdLocal;
    std::string local;
    std::string idRegiao;
    std::string idAreaAviso;
    std::optional<double> latitude;
    std::optional<double> longitude;
};

struct IpmaForecastDay {
    std::string forecastDate;
    int         tMin;
    int         tMax;
    int         precipitaProb;
    int         idWeatherType;
    int         classWindSpeed;
    std::string predWindDir;
};

struct IpmaForecast {
    int                          globalIdLocal;
    std::string                  localName;
    std::vector<IpmaForecastDay> days;
};

struct IpmaWarning {
    std::string awarenessLevelID;   // "yellow", "orange", "red"
    std::string awarenessTypeName;  // "Vento", "Precipitação", etc.
    std::string startTime;
    std::string endTime;
    std::string idAreaAviso;        // area code, e.g. "AVR"
};

struct IpmaWeatherType {
    int         id;
    std::string descPT;
    std::string descEN;
};

struct IpmaWindType {
    int         id;
    std::string descPT;
    std::string descEN;
};

struct IpmaWindDirectionType {
    std::string id;
    std::string labelPT;
    std::string labelEN;
    std::string siglaPT;
    std::string siglaEN;
    std::string descPT;
    std::string descEN;
};

struct IpmaAggregateHour {
    std::string           forecastDateTime;
    std::string           forecastDate;
    std::optional<float>  temperature;
    std::optional<float>  feltTemperature;
    std::optional<int>    precipProbability;
    int                   idWeatherType;
    std::optional<float>  humidity;
    std::optional<float>  windSpeed;
    std::string           windDirection;
    std::optional<float>  seaTotal;
    std::string           waveDirection;
    std::optional<float>  wavePeriod;
    std::optional<float>  peakPeriod;
    std::optional<float>  waterTemperature;
    std::string           dataUpdate;
};

struct IpmaAggregateDay {
    std::string                   forecastDate;
    std::optional<int>            tMin;
    std::optional<int>            tMax;
    std::optional<int>            precipProbability;
    std::optional<float>          uvIndex;
    std::string                   uvInterval;
    int                           idWeatherType;
    std::optional<int>            windClass;
    std::string                   windDirection;
    std::string                   dataUpdate;
    std::vector<IpmaAggregateHour> hours;
};

struct IpmaAggregateForecast {
    int                          globalIdLocal;
    std::string                  localName;
    std::string                  dataUpdate;
    std::vector<IpmaAggregateDay> days;
};

struct IpmaUvEntry {
    int         globalIdLocal;
    float       iUv;
    std::string intervaloHora; // e.g. "13h-15h"
    std::string data;          // date "YYYY-MM-DD"
    int         idPeriodo;
};

struct IpmaSeaLocation {
    int         globalIdLocal;
    std::string local;
    std::optional<double> latitude;
    std::optional<double> longitude;
};

struct IpmaSeaForecastDay {
    std::string forecastDate;
    float       waveHeightMin;
    float       waveHeightMax;
    float       wavePeriodMin;
    float       wavePeriodMax;
    std::string predWaveDir;
    float       sstMin;
    float       sstMax;
};

struct IpmaSeaForecast {
    int                             globalIdLocal;
    std::string                     localName;
    std::vector<IpmaSeaForecastDay> days;
};
