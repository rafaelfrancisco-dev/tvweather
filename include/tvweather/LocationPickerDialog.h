#pragma once

#define Uses_TDialog
#define Uses_TRect
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <vector>

class TListBox;
class TScrollBar;

class LocationPickerDialog : public TDialog {
public:
    explicit LocationPickerDialog(const std::vector<IpmaLocation>& locations,
                                  tvweather::Language              language);

    // Returns the location corresponding to the focused list item.
    // Only valid after execView() returns cmOK.
    IpmaLocation selectedLocation() const;

private:
    std::vector<IpmaLocation> sorted_;
    TListBox*                 listBox_;
};
