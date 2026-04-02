#define Uses_TDialog
#define Uses_TButton
#define Uses_TListBox
#define Uses_TScrollBar
#define Uses_TStringCollection
#define Uses_TRect
#define Uses_TKeys
#include <tvision/tv.h>

#include "tvweather/LocationPickerDialog.h"
#include "tvweather/Localization.h"
#include <algorithm>

// Dialog is 52 wide x 20 tall, centered on screen.
// Interior (inside frame): cols 1-50, rows 1-18.
//   List:    TRect(1, 1, 49, 16) → 48 wide, 15 tall
//   SBar:    TRect(49, 1, 50, 16) → 1 wide, 15 tall
//   Buttons: row 17-18

LocationPickerDialog::LocationPickerDialog(
    const std::vector<IpmaLocation>& locations,
    tvweather::Language              language)
    : TWindowInit(TWindow::initFrame),
      TDialog(TRect(0, 0, 52, 20), tvweather::uiStrings(language).locationDialogTitle),
      sorted_(locations)
{
    const auto& strings = tvweather::uiStrings(language);
    options |= ofCentered;

    std::sort(sorted_.begin(), sorted_.end(),
              [](const IpmaLocation& a, const IpmaLocation& b) {
                  return a.local < b.local;
              });

    auto* sb = new TScrollBar(TRect(49, 1, 50, 16));
    insert(sb);

    listBox_ = new TListBox(TRect(1, 1, 49, 16), 1, sb);
    insert(listBox_);

    auto* sc = new TStringCollection(
        static_cast<short>(sorted_.size() + 1), 10);
    for (const auto& loc : sorted_)
        sc->insert(newStr(loc.local.c_str()));
    listBox_->newList(sc);

    insert(new TButton(TRect(4,  17, 22, 19), strings.openForecast, cmOK,     bfDefault));
    insert(new TButton(TRect(26, 17, 44, 19), strings.cancel,       cmCancel, bfNormal));
}

IpmaLocation LocationPickerDialog::selectedLocation() const
{
    const auto idx = static_cast<int>(listBox_->focused);
    if (idx >= 0 && idx < static_cast<int>(sorted_.size()))
        return sorted_[idx];
    return {};
}
