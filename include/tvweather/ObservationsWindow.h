#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>

class ObservationsWindow : public TWindow {
public:
    ObservationsWindow(const IpmaStationObservation& observation,
                       double                        stationDistanceKm,
                       const std::string&            selectedLocationName,
                       tvweather::Language           language);

    void setLanguage(tvweather::Language language);

private:
    void updateTitle();

    IpmaStationObservation observation_;
    double                 stationDistanceKm_;
    std::string            selectedLocationName_;
    tvweather::Language    language_;
};
