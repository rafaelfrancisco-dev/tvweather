#pragma once

#define Uses_TWindow
#define Uses_TRect
#define Uses_TEvent
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>
#include <vector>

class DividerView;
class ForecastDayListView;
class ForecastDetailPane;
class ForecastHourlyView;
class ForecastWarningsPane;
class TScrollBar;

class ForecastWindow : public TWindow {
public:
    ForecastWindow(const IpmaAggregateForecast&             forecast,
                   const std::vector<IpmaWeatherType>&      weatherTypes,
                   const std::vector<IpmaWindType>&         windTypes,
                   const std::vector<IpmaWindDirectionType>& windDirectionTypes,
                   const std::vector<IpmaWarning>&          warnings,
                   std::string                              warningAreaCode,
                   tvweather::Language                      language);

    void changeBounds(const TRect& bounds) override;
    void handleEvent(TEvent& event) override;
    void setLanguage(tvweather::Language language);
    void sizeLimits(TPoint& min, TPoint& max) override;

private:
    void applySelectionByDate(const std::string& dayDate, const std::string& dateTime);
    void relayout();
    void updateTitle();

    IpmaAggregateForecast               forecast_;
    std::vector<IpmaWeatherType>        weatherTypes_;
    std::vector<IpmaWindType>           windTypes_;
    std::vector<IpmaWindDirectionType>  windDirectionTypes_;
    std::vector<IpmaWarning>            warnings_;
    std::string                         warningAreaCode_;
    tvweather::Language                 language_;
    ForecastDayListView*                dayList_ {nullptr};
    TScrollBar*                         dayScrollBar_ {nullptr};
    DividerView*                        primaryDivider_ {nullptr};
    ForecastHourlyView*                 hourlyView_ {nullptr};
    TScrollBar*                         hourlyScrollBar_ {nullptr};
    DividerView*                        sideDivider_ {nullptr};
    ForecastDetailPane*                 detailPane_ {nullptr};
    DividerView*                        detailDivider_ {nullptr};
    ForecastWarningsPane*               warningsPane_ {nullptr};
};
