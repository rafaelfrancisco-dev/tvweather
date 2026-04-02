#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TScrollBar
#define Uses_TListViewer
#include <tvision/tv.h>

#include "tvweather/SeaWindow.h"
#include "tvweather/Localization.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace
{

constexpr ushort cmSeaDayFocused             = 121;
constexpr int    kSeaHorizontalSplitThreshold = 60;

std::string compactDate(const std::string& d)
{
    if (d.size() >= 10 && d[4] == '-' && d[7] == '-')
        return d.substr(5, 5);
    return d;
}

struct SeaPaneLayout {
    TRect overviewList;
    TRect overviewScrollBar;
    TRect detail;
    TRect divider;
    bool  stacked;
};

// ── Simple divider ────────────────────────────────────────────────────────────

class SeaDivider : public TView {
public:
    SeaDivider(const TRect& bounds, bool vertical) noexcept
        : TView(bounds), vertical_(vertical) {}

    void setVertical(bool v) { vertical_ = v; drawView(); }

    void draw() override
    {
        const TColorAttr color = getColor(1);
        TDrawBuffer b;
        if (vertical_) {
            b.moveChar(0, '|', color, 1);
            for (int y = 0; y < size.y; ++y)
                writeLine(0, y, 1, 1, b);
        } else {
            b.moveChar(0, '-', color, (ushort)size.x);
            writeLine(0, 0, size.x, 1, b);
        }
    }

private:
    bool vertical_;
};

// ── Detail view ───────────────────────────────────────────────────────────────

class SeaDetailView : public TView {
public:
    SeaDetailView(const TRect& bounds,
                  const IpmaSeaForecast& forecast,
                  tvweather::Language    language) noexcept
        : TView(bounds),
          forecast_(forecast),
          language_(language)
    {
        eventMask |= evBroadcast;
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    void draw() override
    {
        const auto& strings = tvweather::uiStrings(language_);
        const TColorAttr color = getColor(1);
        fillView(color);

        if (forecast_.days.empty()) {
            writeTextLine(0, 0, strings.seaEmpty, color);
            return;
        }

        const auto& day = forecast_.days[static_cast<std::size_t>(focusedIndex_)];

        char buf[64];
        const std::vector<std::string> lines {
            day.forecastDate,
            [&]{
                std::snprintf(buf, sizeof(buf), "%s: %.1f - %.1f m",
                    strings.seaWaveHeight,
                    static_cast<double>(day.waveHeightMin),
                    static_cast<double>(day.waveHeightMax));
                return std::string(buf);
            }(),
            [&]{
                std::snprintf(buf, sizeof(buf), "%s: %.1f - %.1f s",
                    strings.seaWavePeriod,
                    static_cast<double>(day.wavePeriodMin),
                    static_cast<double>(day.wavePeriodMax));
                return std::string(buf);
            }(),
            std::string(strings.seaWaveDir) + ": "
                + (day.predWaveDir.empty() ? "-" : day.predWaveDir),
            [&]{
                std::snprintf(buf, sizeof(buf), "%s: %.1f - %.1f\xF8",
                    strings.seaSst,
                    static_cast<double>(day.sstMin),
                    static_cast<double>(day.sstMax));
                return std::string(buf);
            }(),
        };

        int y = 0;
        for (const auto& line : lines) {
            if (y >= size.y)
                break;
            writeTextLine(0, y++, line, color);
        }
    }

    void handleEvent(TEvent& event) override
    {
        TView::handleEvent(event);
        if (event.what == evBroadcast && event.message.command == cmSeaDayFocused) {
            focusedIndex_ = event.message.infoByte;
            drawView();
        }
    }

private:
    void fillView(TColorAttr color)
    {
        if (size.x <= 0 || size.y <= 0) return;
        TDrawBuffer b;
        b.moveChar(0, ' ', color, (ushort)size.x);
        for (int y = 0; y < size.y; ++y)
            writeLine(0, y, size.x, 1, b);
    }

    void writeTextLine(int x, int y, const std::string& text, TColorAttr color)
    {
        if (size.x <= 0 || y < 0 || y >= size.y) return;
        TDrawBuffer b;
        b.moveChar(0, ' ', color, (ushort)size.x);
        b.moveStr(0, text.c_str(), color, (ushort)size.x);
        writeLine(x, y, size.x, 1, b);
    }

    const IpmaSeaForecast& forecast_;
    tvweather::Language    language_;
    uchar                  focusedIndex_ {0};
};

// ── Overview list ─────────────────────────────────────────────────────────────

class SeaOverviewList : public TListViewer {
public:
    SeaOverviewList(const TRect& bounds,
                    TScrollBar*  scrollBar,
                    const IpmaSeaForecast& forecast,
                    tvweather::Language    language) noexcept
        : TListViewer(bounds, 1, nullptr, scrollBar),
          forecast_(forecast),
          language_(language)
    {
        setRange(static_cast<short>(forecast_.days.size()));
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    void getText(char* dest, short item, short maxLen) override
    {
        if (item < 0 || item >= range || maxLen <= 0) {
            if (maxLen > 0) dest[0] = '\0';
            return;
        }
        const auto& day = forecast_.days[static_cast<std::size_t>(item)];
        std::snprintf(dest,
                      static_cast<std::size_t>(maxLen),
                      "%-5s %4.1f-%4.1fm",
                      compactDate(day.forecastDate).c_str(),
                      static_cast<double>(day.waveHeightMin),
                      static_cast<double>(day.waveHeightMax));
    }

    void focusItem(short item) override
    {
        TListViewer::focusItem(item);
        if (owner != nullptr && focused >= 0 && focused < range)
            message(owner, evBroadcast, cmSeaDayFocused, (void*)(size_t)focused);
    }

private:
    const IpmaSeaForecast& forecast_;
    tvweather::Language    language_;
};

// ── Layout helpers ────────────────────────────────────────────────────────────

SeaPaneLayout computeSeaPaneLayout(const TRect& bounds)
{
    const int width  = bounds.b.x - bounds.a.x;
    const int height = bounds.b.y - bounds.a.y;

    if (width >= kSeaHorizontalSplitThreshold) {
        const int listWidth = std::min(20, std::max(16, width / 3));
        const int listRight = bounds.a.x + listWidth;
        return {
            TRect(bounds.a.x, bounds.a.y, listRight, bounds.b.y),
            TRect(listRight, bounds.a.y, listRight + 1, bounds.b.y),
            TRect(listRight + 2, bounds.a.y, bounds.b.x, bounds.b.y),
            TRect(listRight + 1, bounds.a.y, listRight + 2, bounds.b.y),
            false
        };
    }

    const int topHeight = std::min(height / 2, 6);
    const int topBottom = bounds.a.y + topHeight;
    return {
        TRect(bounds.a.x, bounds.a.y, bounds.b.x - 1, topBottom),
        TRect(bounds.b.x - 1, bounds.a.y, bounds.b.x, topBottom),
        TRect(bounds.a.x, topBottom + 1, bounds.b.x, bounds.b.y),
        TRect(bounds.a.x, topBottom, bounds.b.x, topBottom + 1),
        true
    };
}

template <class T>
T* findSeaChild(TView* first, TView* last)
{
    for (TView* view = first; view != nullptr; view = view->nextView()) {
        if (auto* casted = dynamic_cast<T*>(view))
            return casted;
        if (view == last)
            break;
    }
    return nullptr;
}

} // namespace

// ── SeaWindow ─────────────────────────────────────────────────────────────────

SeaWindow::SeaWindow(const IpmaSeaForecast& forecast,
                     tvweather::Language    language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(3, 2, 77, 20), "", wnNoNumber),
      forecast_(forecast),
      language_(language)
{
    options |= ofTileable;
    updateTitle();

    TRect interior = getExtent();
    interior.grow(-1, -1);
    const SeaPaneLayout layout = computeSeaPaneLayout(interior);

    auto* overviewScrollBar = new TScrollBar(layout.overviewScrollBar);
    auto* overviewList = new SeaOverviewList(layout.overviewList,
                                              overviewScrollBar,
                                              forecast_,
                                              language_);
    auto* detailView = new SeaDetailView(layout.detail, forecast_, language_);
    auto* divider    = new SeaDivider(layout.divider, !layout.stacked);

    insert(overviewScrollBar);
    insert(overviewList);
    insert(divider);
    insert(detailView);

    overviewList->select();

    if (!forecast_.days.empty())
        message(this, evBroadcast, cmSeaDayFocused, 0);
}

void SeaWindow::changeBounds(const TRect& bounds)
{
    TWindow::changeBounds(bounds);

    TRect interior = getExtent();
    interior.grow(-1, -1);
    const SeaPaneLayout layout = computeSeaPaneLayout(interior);

    if (auto* list = findSeaChild<SeaOverviewList>(first(), last))
        list->setBounds(layout.overviewList);
    if (auto* sb = findSeaChild<TScrollBar>(first(), last))
        sb->setBounds(layout.overviewScrollBar);
    if (auto* detail = findSeaChild<SeaDetailView>(first(), last))
        detail->setBounds(layout.detail);
    if (auto* div = findSeaChild<SeaDivider>(first(), last)) {
        div->setBounds(layout.divider);
        div->setVertical(!layout.stacked);
    }
}

void SeaWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    if (auto* list = findSeaChild<SeaOverviewList>(first(), last))
        list->setLanguage(language_);
    if (auto* detail = findSeaChild<SeaDetailView>(first(), last))
        detail->setLanguage(language_);

    drawView();
}

void SeaWindow::sizeLimits(TPoint& min, TPoint& max)
{
    TWindow::sizeLimits(min, max);
    min.x = std::max<short>(min.x, 40);
    min.y = std::max<short>(min.y, 12);
}

void SeaWindow::updateTitle()
{
    const auto& strings = tvweather::uiStrings(language_);
    const std::string t = forecast_.localName + strings.seaWindowSuffix;
    delete[] const_cast<char*>(title);
    title = newStr(t.c_str());
}
