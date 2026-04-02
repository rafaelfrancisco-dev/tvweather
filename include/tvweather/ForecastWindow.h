#pragma once

#define Uses_TWindow
#define Uses_TRect
#define Uses_TEvent
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

    void changeBounds(const TRect& bounds) override;
    void handleEvent(TEvent& event) override;
    void setLanguage(tvweather::Language language);
    void sizeLimits(TPoint& min, TPoint& max) override;

private:
    void updateTitle();

    IpmaForecast                 forecast_;
    std::vector<IpmaWeatherType> weatherTypes_;
    tvweather::Language          language_;
};
