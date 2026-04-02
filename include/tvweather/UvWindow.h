#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <vector>

class UvWindow : public TWindow {
public:
    UvWindow(const std::vector<IpmaUvEntry>& entries,
             int globalIdLocal,
             const std::string& localName,
             tvweather::Language language);

    void setLanguage(tvweather::Language language);

private:
    void updateTitle();

    std::vector<IpmaUvEntry> entries_;
    std::string              localName_;
    tvweather::Language      language_;
};
