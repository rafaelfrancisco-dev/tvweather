#pragma once

#define Uses_TDialog
#define Uses_TRect
#define Uses_TEvent
#include <tvision/tv.h>

#include "ipma/Models.h"
#include "tvweather/Language.h"
#include <string>
#include <vector>

class TInputLine;
class TListBox;
class TScrollBar;

class LocationPickerDialog : public TDialog {
public:
    explicit LocationPickerDialog(const std::vector<IpmaLocation>& locations,
                                  tvweather::Language              language);

    // Returns the location corresponding to the focused list item.
    // Only valid after execView() returns cmOK.
    IpmaLocation selectedLocation() const;

    void handleEvent(TEvent& event) override;

private:
    void rebuildList(const std::string& filter);

    std::vector<IpmaLocation> sorted_;
    std::vector<IpmaLocation> filtered_;
    TInputLine*               searchInput_;
    TListBox*                 listBox_;
    std::string               lastFilter_;
};
