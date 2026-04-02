#pragma once

#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include <vector>

class WarningsWindow : public TWindow {
public:
    explicit WarningsWindow(const std::vector<IpmaWarning>& warnings);
};
