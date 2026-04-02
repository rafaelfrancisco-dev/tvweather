#define Uses_TDialog
#define Uses_TButton
#define Uses_TInputLine
#define Uses_TListBox
#define Uses_TScrollBar
#define Uses_TStringCollection
#define Uses_TRect
#define Uses_TKeys
#include <tvision/tv.h>

#include "tvweather/LocationPickerDialog.h"
#include "tvweather/Localization.h"
#include <algorithm>
#include <cctype>
#include <string>

// Dialog is 52 wide x 20 tall, centered on screen.
// Interior (inside frame): cols 1-50, rows 1-18.
//   Search:  TRect(1, 1, 49, 2) →  1 tall, for live filter
//   List:    TRect(1, 2, 49, 16) → 14 tall
//   SBar:    TRect(49, 2, 50, 16) → 1 wide, 14 tall
//   Buttons: rows 17-18

namespace {

std::string toLowerAscii(const std::string& s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

} // namespace

LocationPickerDialog::LocationPickerDialog(
    const std::vector<IpmaLocation>& locations,
    tvweather::Language              language)
    : TWindowInit(TWindow::initFrame),
      TDialog(TRect(0, 0, 52, 20), tvweather::uiStrings(language).locationDialogTitle),
      sorted_(locations),
      filtered_(locations)
{
    const auto& strings = tvweather::uiStrings(language);
    options |= ofCentered;

    std::sort(sorted_.begin(), sorted_.end(),
              [](const IpmaLocation& a, const IpmaLocation& b) {
                  return a.local < b.local;
              });
    filtered_ = sorted_;

    searchInput_ = new TInputLine(TRect(1, 1, 49, 2), 48);
    insert(searchInput_);

    auto* sb = new TScrollBar(TRect(49, 2, 50, 16));
    insert(sb);

    listBox_ = new TListBox(TRect(1, 2, 49, 16), 1, sb);
    insert(listBox_);

    auto* sc = new TStringCollection(
        static_cast<short>(sorted_.size() + 1), 10);
    for (const auto& loc : sorted_)
        sc->insert(newStr(loc.local.c_str()));
    listBox_->newList(sc);

    insert(new TButton(TRect(4,  17, 22, 19), strings.openForecast, cmOK,     bfDefault));
    insert(new TButton(TRect(26, 17, 44, 19), strings.cancel,       cmCancel, bfNormal));

    searchInput_->select();
}

IpmaLocation LocationPickerDialog::selectedLocation() const
{
    const auto idx = static_cast<int>(listBox_->focused);
    if (idx >= 0 && idx < static_cast<int>(filtered_.size()))
        return filtered_[idx];
    return {};
}

void LocationPickerDialog::handleEvent(TEvent& event)
{
    TDialog::handleEvent(event);

    const std::string current(searchInput_->data);
    if (current != lastFilter_) {
        lastFilter_ = current;
        rebuildList(current);
    }
}

void LocationPickerDialog::rebuildList(const std::string& filter)
{
    const std::string lowerFilter = toLowerAscii(filter);
    filtered_.clear();
    for (const auto& loc : sorted_) {
        if (lowerFilter.empty()
            || toLowerAscii(loc.local).find(lowerFilter) != std::string::npos)
            filtered_.push_back(loc);
    }

    auto* sc = new TStringCollection(
        static_cast<short>(filtered_.size() + 1), 10);
    for (const auto& loc : filtered_)
        sc->insert(newStr(loc.local.c_str()));
    listBox_->newList(sc);
}
