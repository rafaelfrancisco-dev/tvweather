#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>
#include <vector>

class SeaWindow : public TWindow {
public:
    SeaWindow(const IpmaSeaForecast& forecast,
              tvweather::Language    language);

    void changeBounds(const TRect& bounds) override;
    void setLanguage(tvweather::Language language);
    void sizeLimits(TPoint& min, TPoint& max) override;

private:
    void updateTitle();

    IpmaSeaForecast     forecast_;
    tvweather::Language language_;
};
