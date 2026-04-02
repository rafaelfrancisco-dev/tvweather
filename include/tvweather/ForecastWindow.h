#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include <string>
#include <vector>

class ForecastWindow : public TWindow {
public:
    ForecastWindow(const IpmaForecast&                forecast,
                   const std::vector<IpmaWeatherType>& weatherTypes);

};
