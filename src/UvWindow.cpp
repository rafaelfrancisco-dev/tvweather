#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TScrollBar
#define Uses_TScroller
#define Uses_TKeys
#include <tvision/tv.h>

#include "tvweather/UvWindow.h"
#include "tvweather/Localization.h"
#include <cstdio>
#include <string>
#include <vector>

namespace
{

// ── UvView ────────────────────────────────────────────────────────────────────
// Scrollable view rendering UV index entries.
// Each visible line: "YYYY-MM-DD  13h-15h  UV: 7.2"

class UvView : public TScroller {
public:
    UvView(const TRect& bounds,
           TScrollBar*  vScrollBar,
           const std::vector<IpmaUvEntry>& entries,
           tvweather::Language             language)
        : TScroller(bounds, nullptr, vScrollBar),
          entries_(entries),
          language_(language)
    {
        const int total = entries_.empty() ? 1 : static_cast<int>(entries_.size());
        setLimit(0, total);
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    void draw() override
    {
        const TColorAttr color = getColor(1);
        TDrawBuffer b;
        int y = 0;

        if (entries_.empty()) {
            b.moveChar(0, ' ', color, (ushort)size.x);
            b.moveStr(1, tvweather::uiStrings(language_).uvEmpty, color,
                      (ushort)(size.x - 2));
            writeLine(0, y++, size.x, 1, b);
        } else {
            const int startLine = delta.y;
            const int endLine   = startLine + size.y;
            int line = 0;

            for (const auto& e : entries_) {
                if (line >= startLine && y < size.y) {
                    b.moveChar(0, ' ', color, (ushort)size.x);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "%-10s  %-9s  UV: %.1f",
                                  e.data.c_str(),
                                  e.intervaloHora.c_str(),
                                  static_cast<double>(e.iUv));
                    b.moveStr(1, buf, color, (ushort)(size.x - 2));
                    writeLine(0, y++, size.x, 1, b);
                }
                ++line;
                if (line >= endLine) break;
            }
        }

        b.moveChar(0, ' ', color, (ushort)size.x);
        while (y < size.y)
            writeLine(0, y++, size.x, 1, b);
    }

private:
    std::vector<IpmaUvEntry> entries_;
    tvweather::Language      language_;
};

} // namespace

// ── UvWindow ──────────────────────────────────────────────────────────────────

UvWindow::UvWindow(const std::vector<IpmaUvEntry>& entries,
                   int                              globalIdLocal,
                   const std::string&               localName,
                   tvweather::Language              language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(4, 3, 76, 21), "", wnNoNumber),
      localName_(localName),
      language_(language)
{
    options |= ofTileable;

    // Filter entries to this location only.
    for (const auto& e : entries)
        if (e.globalIdLocal == globalIdLocal)
            entries_.push_back(e);

    updateTitle();

    auto* sb = new TScrollBar(TRect(size.x - 1, 1, size.x, size.y - 1));
    insert(sb);

    TRect r = getExtent();
    r.grow(-1, -1);
    r.b.x -= 1;
    insert(new UvView(r, sb, entries_, language_));
}

void UvWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    for (TView* view = first(); view != nullptr; view = view->nextView()) {
        if (auto* uvView = dynamic_cast<UvView*>(view)) {
            uvView->setLanguage(language_);
            break;
        }
        if (view == last)
            break;
    }

    drawView();
}

void UvWindow::updateTitle()
{
    const auto& strings = tvweather::uiStrings(language_);
    const std::string t = localName_ + " - " + strings.uvWindowTitle;
    delete[] const_cast<char*>(title);
    title = newStr(t.c_str());
}
