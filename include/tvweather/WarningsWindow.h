#pragma once

#define Uses_TWindow
#define Uses_TRect
#define Uses_TEvent
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <vector>

class WarningsWindow : public TWindow {
public:
    explicit WarningsWindow(const std::vector<IpmaWarning>& warnings,
                            tvweather::Language             language);

    void setLanguage(tvweather::Language language);
    void handleEvent(TEvent& event) override;

private:
    void updateTitle();

    std::vector<IpmaWarning> warnings_;
    tvweather::Language      language_;
};
