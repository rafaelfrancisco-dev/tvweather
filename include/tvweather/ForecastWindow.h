#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>
#include <vector>

class ForecastWindow : public TWindow {
public:
    ForecastWindow(const IpmaForecast&                forecast,
                   const std::vector<IpmaWeatherType>& weatherTypes,
                   tvweather::Language                language);

    void setLanguage(tvweather::Language language);

private:
    void updateTitle();

    IpmaForecast                 forecast_;
    std::vector<IpmaWeatherType> weatherTypes_;
    tvweather::Language          language_;
};
