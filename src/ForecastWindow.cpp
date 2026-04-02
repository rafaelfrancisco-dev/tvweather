#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TScrollBar
#define Uses_TListViewer
#include <tvision/tv.h>

#include "tvweather/ForecastWindow.h"
#include "tvweather/Localization.h"
#include "tvweather/Commands.h"
#include "ipma/Client.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace
{

constexpr ushort cmForecastDayFocused = 120;
constexpr int    kHorizontalSplitThreshold = 62;
constexpr char   cpForecastOverview[] = "\x06\x06\x07\x07\x05";

std::string compactDate(const std::string& forecastDate)
{
    if (forecastDate.size() >= 10
        && forecastDate[4] == '-'
        && forecastDate[7] == '-')
        return forecastDate.substr(5, 5);
    return forecastDate;
}

std::string formatTemperatureValue(int temperature)
{
    return std::to_string(temperature) + "\xF8";
}

std::string formatTemperatureRange(const IpmaForecastDay& day)
{
    return formatTemperatureValue(day.tMin) + " / " + formatTemperatureValue(day.tMax);
}

std::string formatRainValue(int precipitaProb)
{
    return std::to_string(precipitaProb) + "%";
}

std::string formatWindValue(const IpmaForecastDay& day)
{
    if (day.predWindDir.empty())
        return std::to_string(day.classWindSpeed);
    return day.predWindDir + " / " + std::to_string(day.classWindSpeed);
}

struct PaneLayout {
    TRect overviewList;
    TRect overviewScrollBar;
    TRect detail;
    TRect divider;
    bool  stacked;
};

class DividerView : public TView {
public:
    DividerView(const TRect& bounds, bool vertical) noexcept :
        TView(bounds),
        vertical_(vertical)
    {}

    void setVertical(bool vertical)
    {
        vertical_ = vertical;
        drawView();
    }

    void draw() override
    {
        const TColorAttr color = getColor(1);
        TDrawBuffer buffer;
        if (vertical_) {
            buffer.moveChar(0, '|', color, 1);
            for (int y = 0; y < size.y; ++y)
                writeLine(0, y, 1, 1, buffer);
        } else {
            buffer.moveChar(0, '-', color, (ushort)size.x);
            writeLine(0, 0, size.x, 1, buffer);
        }
    }

private:
    bool vertical_;
};

class ForecastDetailView : public TView {
public:
    ForecastDetailView(const TRect& bounds,
                       const IpmaForecast& forecast,
                       const std::vector<IpmaWeatherType>& weatherTypes,
                       tvweather::Language language) noexcept
        : TView(bounds),
          forecast_(forecast),
          weatherTypes_(weatherTypes),
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
            writeTextLine(0, 0, strings.forecastDetails, color);
            writeTextLine(0, 2, strings.forecastEmpty, color);
            return;
        }

        const auto& day = forecast_.days[static_cast<std::size_t>(focusedIndex_)];
        const std::string title = std::string(strings.forecastDetails)
            + " "
            + std::to_string(focusedIndex_ + 1)
            + "/"
            + std::to_string(forecast_.days.size());

        writeTextLine(0, 0, title, color);

        const std::vector<std::string> lines {
            day.forecastDate,
            tvweather::weatherDescriptionForType(weatherTypes_, day.idWeatherType, language_),
            std::string(strings.forecastTemperature) + ": " + formatTemperatureRange(day),
            std::string(strings.forecastRain) + ": " + formatRainValue(day.precipitaProb),
            std::string(strings.forecastRainRisk) + ": "
                + tvweather::rainRiskLabel(day.precipitaProb, language_),
            std::string(strings.forecastWind) + ": " + formatWindValue(day),
            std::string(strings.forecastWindStrength) + ": "
                + tvweather::windStrengthLabel(day.classWindSpeed, language_),
        };

        int y = 2;
        for (const auto& line : lines) {
            if (y >= size.y - 1)
                break;
            writeTextLine(0, y++, line, color);
        }

    }

    void handleEvent(TEvent& event) override
    {
        TView::handleEvent(event);

        if (event.what == evBroadcast && event.message.command == cmForecastDayFocused) {
            focusedIndex_ = event.message.infoByte;
            drawView();
        }
    }

private:
    void fillView(TColorAttr color)
    {
        if (size.x <= 0 || size.y <= 0)
            return;

        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, (ushort)size.x);
        for (int y = 0; y < size.y; ++y)
            writeLine(0, y, size.x, 1, buffer);
    }

    void writeTextLine(int x, int y, const std::string& text, TColorAttr color)
    {
        if (size.x <= 0 || y < 0 || y >= size.y)
            return;

        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, (ushort)size.x);
        buffer.moveStr(0, text.c_str(), color, (ushort)size.x);
        writeLine(x, y, size.x, 1, buffer);
    }

    const IpmaForecast&                forecast_;
    const std::vector<IpmaWeatherType>& weatherTypes_;
    tvweather::Language                language_;
    uchar                              focusedIndex_ {0};
};

class ForecastOverviewList : public TListViewer {
public:
    ForecastOverviewList(const TRect& bounds,
                         TScrollBar* scrollBar,
                         const IpmaForecast& forecast,
                         tvweather::Language language) noexcept
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

    TPalette& getPalette() const override
    {
        static TPalette palette(cpForecastOverview, sizeof(cpForecastOverview) - 1);
        return palette;
    }

    void draw() override
    {
        if (forecast_.days.empty()) {
            const TColorAttr color = getColor(1);
            TDrawBuffer emptyLine;
            TDrawBuffer messageLine;
            emptyLine.moveChar(0, ' ', color, (ushort)size.x);
            messageLine.moveChar(0, ' ', color, (ushort)size.x);
            messageLine.moveStr(0, tvweather::uiStrings(language_).forecastEmpty, color,
                                (ushort)size.x);
            writeLine(0, 0, size.x, 1, messageLine);
            for (int y = 1; y < size.y; ++y)
                writeLine(0, y, size.x, 1, emptyLine);
            return;
        }

        TListViewer::draw();
    }

    void getText(char* dest, short item, short maxLen) override
    {
        if (item < 0 || item >= range || maxLen <= 0) {
            if (maxLen > 0)
                dest[0] = '\0';
            return;
        }

        const auto& day = forecast_.days[static_cast<std::size_t>(item)];
        std::snprintf(dest,
                      static_cast<std::size_t>(maxLen),
                      "%-5s %2d/%-2d %3d%%",
                      compactDate(day.forecastDate).c_str(),
                      day.tMin,
                      day.tMax,
                      day.precipitaProb);
    }

    void focusItem(short item) override
    {
        TListViewer::focusItem(item);
        if (owner != nullptr && focused >= 0 && focused < range)
            message(owner, evBroadcast, cmForecastDayFocused, (void*)(size_t)focused);
    }

private:
    const IpmaForecast& forecast_;
    tvweather::Language language_;
};

PaneLayout computePaneLayout(const TRect& bounds)
{
    const int width = bounds.b.x - bounds.a.x;
    const int height = bounds.b.y - bounds.a.y;

    if (width >= kHorizontalSplitThreshold) {
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

    const int topHeight = std::min(height / 2, 7);
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
T* findChild(TView* first, TView* last)
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

ForecastWindow::ForecastWindow(const IpmaForecast&                forecast,
                               const std::vector<IpmaWeatherType>& weatherTypes,
                               tvweather::Language                language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(2, 1, 80, 19), "", wnNoNumber),
      forecast_(forecast),
      weatherTypes_(weatherTypes),
      language_(language)
{
    options |= ofTileable;
    updateTitle();

    TRect interior = getExtent();
    interior.grow(-1, -1);
    const PaneLayout layout = computePaneLayout(interior);

    auto* overviewScrollBar = new TScrollBar(layout.overviewScrollBar);
    auto* overviewList = new ForecastOverviewList(layout.overviewList,
                                                  overviewScrollBar,
                                                  forecast_,
                                                  language_);
    auto* detailView = new ForecastDetailView(layout.detail,
                                              forecast_,
                                              weatherTypes_,
                                              language_);
    auto* divider = new DividerView(layout.divider, !layout.stacked);

    insert(overviewScrollBar);
    insert(overviewList);
    insert(divider);
    insert(detailView);

    overviewList->select();

    if (!forecast_.days.empty())
        message(this, evBroadcast, cmForecastDayFocused, 0);
}

void ForecastWindow::changeBounds(const TRect& bounds)
{
    TWindow::changeBounds(bounds);

    TRect interior = getExtent();
    interior.grow(-1, -1);
    const PaneLayout layout = computePaneLayout(interior);

    if (auto* overviewList = findChild<ForecastOverviewList>(first(), last))
        overviewList->setBounds(layout.overviewList);
    if (auto* overviewScrollBar = findChild<TScrollBar>(first(), last))
        overviewScrollBar->setBounds(layout.overviewScrollBar);
    if (auto* detailView = findChild<ForecastDetailView>(first(), last))
        detailView->setBounds(layout.detail);
    if (auto* divider = findChild<DividerView>(first(), last)) {
        divider->setBounds(layout.divider);
        divider->setVertical(!layout.stacked);
    }
}

void ForecastWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    if (auto* overviewList = findChild<ForecastOverviewList>(first(), last))
        overviewList->setLanguage(language_);
    if (auto* detailView = findChild<ForecastDetailView>(first(), last))
        detailView->setLanguage(language_);

    drawView();
}

void ForecastWindow::sizeLimits(TPoint& min, TPoint& max)
{
    TWindow::sizeLimits(min, max);
    min.x = std::max<short>(min.x, 44);
    min.y = std::max<short>(min.y, 14);
}

void ForecastWindow::handleEvent(TEvent& event)
{
    TWindow::handleEvent(event);

    if (event.what == evCommand && event.message.command == cmRefresh) {
        forecast_ = IpmaClient::fetchForecast(forecast_.globalIdLocal, forecast_.localName);

        if (auto* list = findChild<ForecastOverviewList>(first(), last))
            list->setRange(static_cast<short>(forecast_.days.size()));

        if (!forecast_.days.empty())
            message(this, evBroadcast, cmForecastDayFocused, 0);

        drawView();
        clearEvent(event);
    }
}

void ForecastWindow::updateTitle()
{
    delete[] const_cast<char*>(title);
    title = newStr(tvweather::forecastWindowTitle(forecast_.localName, language_).c_str());
}
