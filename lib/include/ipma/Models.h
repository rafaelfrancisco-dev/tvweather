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
