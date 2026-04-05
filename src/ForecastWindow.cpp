#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TScrollBar
#define Uses_TListViewer
#define Uses_TScroller
#define Uses_MsgBox
#include <tvision/tv.h>

#include "tvweather/ForecastWindow.h"
#include "tvweather/Localization.h"
#include "tvweather/Commands.h"
#include "ipma/Client.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr ushort cmForecastDayFocused  = 120;
constexpr ushort cmForecastHourFocused = 121;
constexpr uchar  kNoHourFocused        = 255;
constexpr int    kWideLayoutThreshold  = 104;
constexpr int    kMidLayoutThreshold   = 82;
constexpr char   cpWindowPane[]        = "\x06";
constexpr char   cpWindowList[]        = "\x06\x06\x07\x07\x05";

struct ForecastWindowLayout {
    TRect dayList;
    TRect dayScrollBar;
    TRect primaryDivider;
    TRect hourlyView;
    TRect hourlyScrollBar;
    TRect sideDivider;
    TRect detailPane;
    TRect detailDivider;
    TRect warningsPane;
};

std::string compactDate(const std::string& value)
{
    if (value.size() >= 10 && value[4] == '-' && value[7] == '-')
        return value.substr(5, 5);
    return value;
}

std::string hourLabelFromDateTime(const std::string& value)
{
    if (value.size() >= 13)
        return value.substr(11, 2) + "H";
    return "--";
}

int weekdayFromDate(const std::string& value)
{
    if (value.size() < 10)
        return -1;

    const int year  = std::stoi(value.substr(0, 4));
    int month = std::stoi(value.substr(5, 2));
    const int day   = std::stoi(value.substr(8, 2));

    int adjustedYear = year;
    if (month < 3) {
        month += 12;
        --adjustedYear;
    }

    const int century = adjustedYear / 100;
    const int yearPart = adjustedYear % 100;
    const int h = (day + (13 * (month + 1)) / 5 + yearPart + yearPart / 4
                   + century / 4 + 5 * century) % 7;
    return (h + 5) % 7;
}

const char* weekdayShortName(const std::string& date, tvweather::Language language)
{
    static constexpr const char* englishDays[] {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    static constexpr const char* portugueseDays[] {
        "Seg", "Ter", "Qua", "Qui", "Sex", "Sab", "Dom"
    };

    const int weekday = weekdayFromDate(date);
    if (weekday < 0 || weekday > 6)
        return "---";

    return language == tvweather::Language::Portuguese
        ? portugueseDays[weekday]
        : englishDays[weekday];
}

std::string formatOptionalInt(const std::optional<int>& value,
                              const char* suffix = "")
{
    if (!value.has_value())
        return "--";
    return std::to_string(*value) + suffix;
}

std::string formatOptionalFloat(const std::optional<float>& value,
                                int precision = 1,
                                const char* suffix = "")
{
    if (!value.has_value())
        return "--";

    char buffer[32];
    std::snprintf(buffer,
                  sizeof(buffer),
                  precision == 0 ? "%.0f%s" : "%.1f%s",
                  static_cast<double>(*value),
                  suffix);
    return buffer;
}

std::string formatTemperaturePair(const IpmaAggregateDay& day)
{
    return formatOptionalInt(day.tMin, "\xF8")
        + " / "
        + formatOptionalInt(day.tMax, "\xF8");
}

std::string warningLevel(const std::string& level)
{
    std::string upper = level;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upper;
}

std::string trimTimestamp(const std::string& value)
{
    if (value.size() >= 16)
        return value.substr(0, 16);
    return value;
}

std::vector<std::string> wrapText(const std::string& text, int width)
{
    if (width <= 0)
        return {};

    std::vector<std::string> lines;
    std::size_t position = 0;

    while (position < text.size()) {
        while (position < text.size() && text[position] == ' ')
            ++position;
        if (position >= text.size())
            break;

        const std::size_t remaining = text.size() - position;
        if (static_cast<int>(remaining) <= width) {
            lines.push_back(text.substr(position));
            break;
        }

        std::size_t breakPos = text.rfind(' ', position + static_cast<std::size_t>(width));
        if (breakPos == std::string::npos || breakPos < position)
            breakPos = position + static_cast<std::size_t>(width);

        lines.push_back(text.substr(position, breakPos - position));
        position = breakPos;
    }

    if (lines.empty())
        lines.push_back({});

    return lines;
}

std::string warningTimeRange(const IpmaWarning& warning,
                             tvweather::Language language)
{
    const auto& strings = tvweather::uiStrings(language);
    return std::string(strings.warningsFrom)
        + " "
        + trimTimestamp(warning.startTime)
        + "  "
        + strings.warningsTo
        + " "
        + trimTimestamp(warning.endTime);
}

ForecastWindowLayout computeLayout(const TRect& bounds)
{
    const int width = bounds.b.x - bounds.a.x;
    const int height = bounds.b.y - bounds.a.y;

    if (width >= kWideLayoutThreshold) {
        const int dayWidth = std::min(34, std::max(28, width / 4));
        const int sideWidth = std::min(34, std::max(28, width / 3));
        const int leftDividerX = bounds.a.x + dayWidth + 1;
        const int sideStart = bounds.b.x - sideWidth;
        const int hourlyRight = sideStart - 2;
        const int detailHeight = std::max(8, height / 2);

        return {
            TRect(bounds.a.x, bounds.a.y, bounds.a.x + dayWidth, bounds.b.y),
            TRect(bounds.a.x + dayWidth, bounds.a.y, bounds.a.x + dayWidth + 1, bounds.b.y),
            TRect(leftDividerX, bounds.a.y, leftDividerX + 1, bounds.b.y),
            TRect(leftDividerX + 1, bounds.a.y, hourlyRight, bounds.b.y),
            TRect(hourlyRight, bounds.a.y, hourlyRight + 1, bounds.b.y),
            TRect(hourlyRight + 1, bounds.a.y, hourlyRight + 2, bounds.b.y),
            TRect(hourlyRight + 2, bounds.a.y, bounds.b.x, bounds.a.y + detailHeight),
            TRect(hourlyRight + 2, bounds.a.y + detailHeight, bounds.b.x, bounds.a.y + detailHeight + 1),
            TRect(hourlyRight + 2, bounds.a.y + detailHeight + 1, bounds.b.x, bounds.b.y)
        };
    }

    if (width >= kMidLayoutThreshold) {
        const int topHeight = std::min(7, std::max(5, height / 4));
        const int detailWidth = std::min(30, std::max(24, width / 3));
        const int hourlyRight = bounds.b.x - detailWidth - 2;

        return {
            TRect(bounds.a.x, bounds.a.y, bounds.b.x - 1, bounds.a.y + topHeight),
            TRect(bounds.b.x - 1, bounds.a.y, bounds.b.x, bounds.a.y + topHeight),
            TRect(bounds.a.x, bounds.a.y + topHeight, bounds.b.x, bounds.a.y + topHeight + 1),
            TRect(bounds.a.x, bounds.a.y + topHeight + 1, hourlyRight, bounds.b.y),
            TRect(hourlyRight, bounds.a.y + topHeight + 1, hourlyRight + 1, bounds.b.y),
            TRect(hourlyRight + 1, bounds.a.y + topHeight + 1, hourlyRight + 2, bounds.b.y),
            TRect(hourlyRight + 2, bounds.a.y + topHeight + 1, bounds.b.x, bounds.a.y + topHeight + 9),
            TRect(hourlyRight + 2, bounds.a.y + topHeight + 9, bounds.b.x, bounds.a.y + topHeight + 10),
            TRect(hourlyRight + 2, bounds.a.y + topHeight + 10, bounds.b.x, bounds.b.y)
        };
    }

    const int dayHeight = std::min(6, std::max(4, height / 4));
    const int bottomHeight = std::max(7, height / 3);
    const int middleBottom = bounds.b.y - bottomHeight - 1;
    const int detailWidth = width / 2;

    return {
        TRect(bounds.a.x, bounds.a.y, bounds.b.x - 1, bounds.a.y + dayHeight),
        TRect(bounds.b.x - 1, bounds.a.y, bounds.b.x, bounds.a.y + dayHeight),
        TRect(bounds.a.x, bounds.a.y + dayHeight, bounds.b.x, bounds.a.y + dayHeight + 1),
        TRect(bounds.a.x, bounds.a.y + dayHeight + 1, bounds.b.x - 1, middleBottom),
        TRect(bounds.b.x - 1, bounds.a.y + dayHeight + 1, bounds.b.x, middleBottom),
        TRect(bounds.a.x + detailWidth, middleBottom + 1, bounds.a.x + detailWidth + 1, bounds.b.y),
        TRect(bounds.a.x, middleBottom + 1, bounds.a.x + detailWidth, bounds.b.y),
        TRect(0, 0, 0, 0),
        TRect(bounds.a.x + detailWidth + 1, middleBottom + 1, bounds.b.x, bounds.b.y)
    };
}

class PaneView : public TView {
public:
    using TView::TView;

protected:
    void fillView(TColorAttr color)
    {
        if (size.x <= 0 || size.y <= 0)
            return;

        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        for (int y = 0; y < size.y; ++y)
            writeLine(0, y, size.x, 1, buffer);
    }

    void writeTextLine(int y, const std::string& text, TColorAttr color)
    {
        if (size.x <= 0 || y < 0 || y >= size.y)
            return;

        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        buffer.moveStr(0, text.c_str(), color, static_cast<ushort>(size.x));
        writeLine(0, y, size.x, 1, buffer);
    }
};

} // namespace

class DividerView : public PaneView {
public:
    DividerView(const TRect& bounds, bool vertical) noexcept :
        PaneView(bounds),
        vertical_(vertical)
    {}

    void setVertical(bool vertical)
    {
        vertical_ = vertical;
        drawView();
    }

    void draw() override
    {
        if (size.x <= 0 || size.y <= 0)
            return;

        const TColorAttr color = getColor(1);
        TDrawBuffer buffer;
        if (vertical_) {
            buffer.moveChar(0, '|', color, 1);
            for (int y = 0; y < size.y; ++y)
                writeLine(0, y, 1, 1, buffer);
        } else {
            buffer.moveChar(0, '-', color, static_cast<ushort>(size.x));
            writeLine(0, 0, size.x, 1, buffer);
        }
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowPane, sizeof(cpWindowPane) - 1);
        return palette;
    }

private:
    bool vertical_;
};

class ForecastDayListView : public TListViewer {
public:
    ForecastDayListView(const TRect& bounds,
                        TScrollBar* scrollBar,
                        const IpmaAggregateForecast& forecast,
                        const std::vector<IpmaWeatherType>& weatherTypes,
                        const std::vector<IpmaWindDirectionType>& windDirectionTypes,
                        tvweather::Language language) noexcept
        : TListViewer(bounds, 1, nullptr, scrollBar),
          forecast_(forecast),
          weatherTypes_(weatherTypes),
          windDirectionTypes_(windDirectionTypes),
          language_(language)
    {
        syncData();
    }

    void syncData()
    {
        setRange(static_cast<short>(forecast_.days.size()));
        if (range > 0) {
            if (focused >= range)
                focusItem(range - 1);
            else
                focusItem(std::max<short>(focused, 0));
        } else
            focused = 0;
        drawView();
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    std::string currentDayDate() const
    {
        if (focused < 0 || focused >= range)
            return {};
        return forecast_.days[static_cast<std::size_t>(focused)].forecastDate;
    }

    void selectDayByDate(const std::string& dayDate)
    {
        short target = 0;
        if (!dayDate.empty()) {
            for (std::size_t index = 0; index < forecast_.days.size(); ++index) {
                if (forecast_.days[index].forecastDate == dayDate) {
                    target = static_cast<short>(index);
                    break;
                }
            }
        }
        if (range > 0)
            focusItem(target);
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowList, sizeof(cpWindowList) - 1);
        return palette;
    }

    void draw() override
    {
        if (forecast_.days.empty()) {
            const TColorAttr color = getColor(1);
            TDrawBuffer buffer;
            buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
            buffer.moveStr(0, tvweather::uiStrings(language_).forecastEmpty, color,
                           static_cast<ushort>(size.x));
            writeLine(0, 0, size.x, 1, buffer);
            for (int y = 1; y < size.y; ++y)
                writeLine(0, y, size.x, 1, buffer);
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
        const std::string row = std::string(weekdayShortName(day.forecastDate, language_))
            + " "
            + compactDate(day.forecastDate)
            + " "
            + formatOptionalInt(day.tMin)
            + "/"
            + formatOptionalInt(day.tMax)
            + " "
            + formatOptionalInt(day.precipProbability, "%")
            + " "
            + tvweather::windDirectionLabelForCode(windDirectionTypes_, day.windDirection, language_)
            + " "
            + tvweather::weatherDescriptionForType(weatherTypes_, day.idWeatherType, language_);

        std::snprintf(dest, static_cast<std::size_t>(maxLen), "%s", row.c_str());
    }

    void focusItem(short item) override
    {
        TListViewer::focusItem(item);
        if (owner != nullptr && focused >= 0 && focused < range)
            message(owner, evBroadcast, cmForecastDayFocused,
                    reinterpret_cast<void*>(static_cast<std::size_t>(focused)));
    }

private:
    const IpmaAggregateForecast&             forecast_;
    const std::vector<IpmaWeatherType>&      weatherTypes_;
    const std::vector<IpmaWindDirectionType>& windDirectionTypes_;
    tvweather::Language                      language_;
};

class ForecastHourlyView : public TScroller {
public:
    ForecastHourlyView(const TRect& bounds,
                       TScrollBar* scrollBar,
                       const IpmaAggregateForecast& forecast,
                       const std::vector<IpmaWeatherType>& weatherTypes,
                       const std::vector<IpmaWindDirectionType>& windDirectionTypes,
                       tvweather::Language language)
        : TScroller(bounds, nullptr, scrollBar),
          forecast_(forecast),
          weatherTypes_(weatherTypes),
          windDirectionTypes_(windDirectionTypes),
          language_(language)
    {
        options |= ofSelectable;
        eventMask |= evBroadcast;
        syncData();
    }

    void syncData()
    {
        if (forecast_.days.empty()) {
            dayIndex_ = 0;
            hourIndex_ = -1;
            setLimit(0, 1);
            scrollTo(0, 0);
            drawView();
            return;
        }

        if (dayIndex_ >= static_cast<int>(forecast_.days.size()))
            dayIndex_ = static_cast<int>(forecast_.days.size()) - 1;

        const auto& hours = currentHours();
        if (hours.empty())
            hourIndex_ = -1;
        else if (hourIndex_ < 0 || hourIndex_ >= static_cast<int>(hours.size()))
            hourIndex_ = 0;

        const int limitY = std::max<int>(1, static_cast<int>(hours.size()));
        setLimit(0, limitY);
        ensureHourVisible();
        broadcastFocusedHour();
        drawView();
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    std::string currentHourDateTime() const
    {
        const auto& hours = currentHours();
        if (hourIndex_ < 0 || hourIndex_ >= static_cast<int>(hours.size()))
            return {};
        return hours[static_cast<std::size_t>(hourIndex_)].forecastDateTime;
    }

    void selectHourByDateTime(const std::string& dateTime)
    {
        const auto& hours = currentHours();
        if (hours.empty()) {
            hourIndex_ = -1;
            scrollTo(0, 0);
            broadcastFocusedHour();
            drawView();
            return;
        }

        int target = 0;
        if (!dateTime.empty()) {
            for (std::size_t index = 0; index < hours.size(); ++index) {
                if (hours[index].forecastDateTime == dateTime) {
                    target = static_cast<int>(index);
                    break;
                }
            }
        }
        setHourIndex(target);
    }

    void draw() override
    {
        const TColorAttr color = getColor(1);
        const TColorAttr headerColor = getColor(5);
        fillView(color);

        writeLineText(0, headerRow(), headerColor);
        if (size.y <= 1)
            return;

        const auto& hours = currentHours();
        if (hours.empty()) {
            writeLineText(1, tvweather::uiStrings(language_).forecastEmpty, color);
            return;
        }

        const int visibleRows = std::max(0, size.y - 1);
        for (int row = 0; row < visibleRows; ++row) {
            const int index = delta.y + row;
            if (index >= static_cast<int>(hours.size()))
                break;

            const bool selected = index == hourIndex_;
            TColorAttr rowColor = color;
            if (selected)
                rowColor = static_cast<TColorAttr>(
                    getState(sfFocused) != 0 ? getColor(4) : getColor(3));
            writeLineText(row + 1, rowText(hours[static_cast<std::size_t>(index)]), rowColor);
        }
    }

    void handleEvent(TEvent& event) override
    {
        TScroller::handleEvent(event);

        if (event.what == evBroadcast && event.message.command == cmForecastDayFocused) {
            dayIndex_ = static_cast<int>(event.message.infoByte);
            const auto& hours = currentHours();
            hourIndex_ = hours.empty() ? -1 : 0;
            const int limitY = std::max<int>(1, static_cast<int>(hours.size()));
            setLimit(0, limitY);
            scrollTo(0, 0);
            broadcastFocusedHour();
            drawView();
            return;
        }

        if (event.what != evKeyDown || getState(sfFocused) == 0)
            return;

        const auto& hours = currentHours();
        if (hours.empty())
            return;

        switch (event.keyDown.keyCode) {
        case kbUp:
            setHourIndex(hourIndex_ - 1);
            clearEvent(event);
            break;
        case kbDown:
            setHourIndex(hourIndex_ + 1);
            clearEvent(event);
            break;
        case kbPgUp:
            setHourIndex(hourIndex_ - std::max(1, size.y - 2));
            clearEvent(event);
            break;
        case kbPgDn:
            setHourIndex(hourIndex_ + std::max(1, size.y - 2));
            clearEvent(event);
            break;
        case kbHome:
            setHourIndex(0);
            clearEvent(event);
            break;
        case kbEnd:
            setHourIndex(static_cast<int>(hours.size()) - 1);
            clearEvent(event);
            break;
        default:
            break;
        }
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowList, sizeof(cpWindowList) - 1);
        return palette;
    }

private:
    void fillView(TColorAttr color)
    {
        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        for (int y = 0; y < size.y; ++y)
            writeLine(0, y, size.x, 1, buffer);
    }

    void writeLineText(int y, const std::string& text, TColorAttr color)
    {
        if (y < 0 || y >= size.y)
            return;

        TDrawBuffer buffer;
        buffer.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        buffer.moveStr(0, text.c_str(), color, static_cast<ushort>(size.x));
        writeLine(0, y, size.x, 1, buffer);
    }

    const std::vector<IpmaAggregateHour>& currentHours() const
    {
        static const std::vector<IpmaAggregateHour> empty;
        if (forecast_.days.empty() || dayIndex_ < 0
            || dayIndex_ >= static_cast<int>(forecast_.days.size()))
            return empty;
        return forecast_.days[static_cast<std::size_t>(dayIndex_)].hours;
    }

    void setHourIndex(int index)
    {
        const auto& hours = currentHours();
        if (hours.empty()) {
            hourIndex_ = -1;
            scrollTo(0, 0);
            broadcastFocusedHour();
            drawView();
            return;
        }

        hourIndex_ = std::max(0, std::min(index, static_cast<int>(hours.size()) - 1));
        ensureHourVisible();
        broadcastFocusedHour();
        drawView();
    }

    void ensureHourVisible()
    {
        if (hourIndex_ < 0) {
            scrollTo(0, 0);
            return;
        }

        const int visibleRows = std::max(1, size.y - 1);
        int targetDelta = delta.y;
        if (hourIndex_ < targetDelta)
            targetDelta = hourIndex_;
        else if (hourIndex_ >= targetDelta + visibleRows)
            targetDelta = hourIndex_ - visibleRows + 1;
        scrollTo(0, std::max(0, targetDelta));
    }

    void broadcastFocusedHour()
    {
        if (owner == nullptr)
            return;

        const uchar focused = hourIndex_ < 0 ? kNoHourFocused
                                             : static_cast<uchar>(hourIndex_);
        message(owner, evBroadcast, cmForecastHourFocused,
                reinterpret_cast<void*>(static_cast<std::size_t>(focused)));
    }

    std::string headerRow() const
    {
        const auto& strings = tvweather::uiStrings(language_);
        std::string header = std::string(strings.forecastHour) + "  P%   T/F";
        if (size.x >= 28)
            header += "    Wind";
        if (size.x >= 42)
            header += "      Sea";
        if (size.x >= 54)
            header += "  H2O";
        if (size.x >= 66)
            header += "  Cond";
        return header;
    }

    std::string rowText(const IpmaAggregateHour& hour) const
    {
        char buffer[256];
        const std::string hourLabel = hourLabelFromDateTime(hour.forecastDateTime);
        const std::string precip = formatOptionalInt(hour.precipProbability, "%");
        const std::string temperatures = formatOptionalFloat(hour.temperature, 0)
            + "/"
            + formatOptionalFloat(hour.feltTemperature, 0);
        const std::string wind = formatOptionalFloat(hour.windSpeed, 0)
            + " "
            + tvweather::windDirectionLabelForCode(windDirectionTypes_, hour.windDirection, language_);

        int written = std::snprintf(buffer,
                                    sizeof(buffer),
                                    "%-4s %-4s %-7s %-10s",
                                    hourLabel.c_str(),
                                    precip.c_str(),
                                    temperatures.c_str(),
                                    wind.c_str());

        if (size.x >= 42) {
            const std::string sea = formatOptionalFloat(hour.seaTotal, 1)
                + " "
                + tvweather::windDirectionLabelForCode(windDirectionTypes_,
                                                       hour.waveDirection,
                                                       language_);
            written += std::snprintf(buffer + written,
                                     sizeof(buffer) - static_cast<std::size_t>(written),
                                     " %-10s",
                                     sea.c_str());
        }

        if (size.x >= 54) {
            const std::string water = formatOptionalFloat(hour.waterTemperature, 0);
            written += std::snprintf(buffer + written,
                                     sizeof(buffer) - static_cast<std::size_t>(written),
                                     " %-4s",
                                     water.c_str());
        }

        if (size.x >= 66) {
            const std::string condition = tvweather::weatherDescriptionForType(
                weatherTypes_, hour.idWeatherType, language_);
            std::snprintf(buffer + written,
                          sizeof(buffer) - static_cast<std::size_t>(written),
                          " %s",
                          condition.c_str());
        }

        return buffer;
    }

    const IpmaAggregateForecast&             forecast_;
    const std::vector<IpmaWeatherType>&      weatherTypes_;
    const std::vector<IpmaWindDirectionType>& windDirectionTypes_;
    tvweather::Language                      language_;
    int                                      dayIndex_ {0};
    int                                      hourIndex_ {-1};
};

class ForecastDetailPane : public PaneView {
public:
    ForecastDetailPane(const TRect& bounds,
                       const IpmaAggregateForecast& forecast,
                       const std::vector<IpmaWeatherType>& weatherTypes,
                       const std::vector<IpmaWindType>& windTypes,
                       const std::vector<IpmaWindDirectionType>& windDirectionTypes,
                       tvweather::Language language) noexcept
        : PaneView(bounds),
          forecast_(forecast),
          weatherTypes_(weatherTypes),
          windTypes_(windTypes),
          windDirectionTypes_(windDirectionTypes),
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
            writeTextLine(0, strings.forecastEmpty, color);
            return;
        }

        const auto& day = forecast_.days[static_cast<std::size_t>(dayIndex_)];
        int y = 0;
        const std::string rainRisk = day.precipProbability.has_value()
            ? tvweather::rainRiskLabel(*day.precipProbability, language_)
            : "--";
        const std::string windStrength = day.windClass.has_value()
            ? tvweather::windTypeDescriptionForId(windTypes_, *day.windClass, language_)
            : "--";

        writeTextLine(y++, std::string(strings.forecastDate) + ": " + day.forecastDate, color);
        writeTextLine(y++, std::string(strings.forecastTemperature) + ": " + formatTemperaturePair(day), color);
        writeTextLine(y++, std::string(strings.forecastRain) + ": "
                          + formatOptionalInt(day.precipProbability, "%"), color);
        writeTextLine(y++, std::string(strings.forecastRainRisk) + ": " + rainRisk, color);
        writeTextLine(y++, std::string(strings.forecastWindStrength) + ": " + windStrength, color);
        writeTextLine(y++, std::string(strings.forecastDirection) + ": "
                          + tvweather::windDirectionDescriptionForCode(
                              windDirectionTypes_, day.windDirection, language_), color);

        if (day.uvIndex.has_value()) {
            std::string uvLine = std::string(strings.forecastUv) + ": "
                + formatOptionalFloat(day.uvIndex, 1);
            if (!day.uvInterval.empty())
                uvLine += " (" + day.uvInterval + ")";
            writeTextLine(y++, uvLine, color);
        }

        if (!forecast_.dataUpdate.empty())
            writeTextLine(y++, std::string(strings.forecastUpdated) + ": "
                              + trimTimestamp(forecast_.dataUpdate), color);
        writeTextLine(y++, strings.forecastUtcNotice, color);

        if (y < size.y)
            ++y;

        writeTextLine(y++, strings.forecastSelectedHour, color);
        const auto* hour = currentHour();
        if (hour == nullptr) {
            writeTextLine(y, strings.forecastEmpty, color);
            return;
        }

        const std::vector<std::string> lines {
            std::string(strings.forecastHour) + ": " + hourLabelFromDateTime(hour->forecastDateTime),
            std::string(strings.forecastConditions) + ": "
                + tvweather::weatherDescriptionForType(weatherTypes_, hour->idWeatherType, language_),
            std::string(strings.forecastTemperature) + ": "
                + formatOptionalFloat(hour->temperature, 1, "\xF8"),
            std::string(strings.forecastFeelsLike) + ": "
                + formatOptionalFloat(hour->feltTemperature, 1, "\xF8"),
            std::string(strings.forecastHumidity) + ": "
                + formatOptionalFloat(hour->humidity, 0, "%"),
            std::string(strings.forecastRain) + ": "
                + formatOptionalInt(hour->precipProbability, "%"),
            std::string(strings.forecastWindSpeed) + ": "
                + formatOptionalFloat(hour->windSpeed, 0, " km/h"),
            std::string(strings.forecastDirection) + ": "
                + tvweather::windDirectionDescriptionForCode(
                    windDirectionTypes_, hour->windDirection, language_),
            std::string(strings.forecastSeaState) + ": "
                + formatOptionalFloat(hour->seaTotal, 1, " m"),
            std::string(strings.forecastWaveDirection) + ": "
                + tvweather::windDirectionDescriptionForCode(
                    windDirectionTypes_, hour->waveDirection, language_),
            std::string(strings.forecastWavePeriod) + ": "
                + formatOptionalFloat(hour->wavePeriod, 1, " s"),
            std::string(strings.forecastPeakPeriod) + ": "
                + formatOptionalFloat(hour->peakPeriod, 1, " s"),
            std::string(strings.forecastWaterTemperature) + ": "
                + formatOptionalFloat(hour->waterTemperature, 1, "\xF8")
        };

        for (const auto& line : lines) {
            if (y >= size.y)
                break;
            writeTextLine(y++, line, color);
        }
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowPane, sizeof(cpWindowPane) - 1);
        return palette;
    }

    void handleEvent(TEvent& event) override
    {
        PaneView::handleEvent(event);

        if (event.what == evBroadcast && event.message.command == cmForecastDayFocused) {
            dayIndex_ = static_cast<int>(event.message.infoByte);
            if (currentHours().empty())
                hourIndex_ = -1;
            drawView();
        } else if (event.what == evBroadcast
                   && event.message.command == cmForecastHourFocused) {
            hourIndex_ = event.message.infoByte == kNoHourFocused
                ? -1
                : static_cast<int>(event.message.infoByte);
            drawView();
        }
    }

private:
    const std::vector<IpmaAggregateHour>& currentHours() const
    {
        static const std::vector<IpmaAggregateHour> empty;
        if (forecast_.days.empty() || dayIndex_ < 0
            || dayIndex_ >= static_cast<int>(forecast_.days.size()))
            return empty;
        return forecast_.days[static_cast<std::size_t>(dayIndex_)].hours;
    }

    const IpmaAggregateHour* currentHour() const
    {
        const auto& hours = currentHours();
        if (hourIndex_ < 0 || hourIndex_ >= static_cast<int>(hours.size()))
            return nullptr;
        return &hours[static_cast<std::size_t>(hourIndex_)];
    }

    const IpmaAggregateForecast&             forecast_;
    const std::vector<IpmaWeatherType>&      weatherTypes_;
    const std::vector<IpmaWindType>&         windTypes_;
    const std::vector<IpmaWindDirectionType>& windDirectionTypes_;
    tvweather::Language                      language_;
    int                                      dayIndex_ {0};
    int                                      hourIndex_ {-1};
};

class ForecastWarningsPane : public PaneView {
public:
    ForecastWarningsPane(const TRect& bounds,
                         const std::vector<IpmaWarning>& warnings,
                         tvweather::Language language) noexcept
        : PaneView(bounds),
          warnings_(warnings),
          language_(language)
    {}

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

        int y = 0;
        if (warnings_.empty()) {
            writeWrappedText(y, strings.forecastWarningsEmpty, color);
            return;
        }

        for (const auto& warning : warnings_) {
            if (y >= size.y)
                break;
            writeWrappedText(y,
                             warningLevel(warning.awarenessLevelID)
                                 + " " + warning.awarenessTypeName,
                             color);
            if (y >= size.y)
                break;
            writeWrappedText(y, warningTimeRange(warning, language_), color);
            if (y >= size.y)
                break;
            ++y;
        }
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowPane, sizeof(cpWindowPane) - 1);
        return palette;
    }

private:
    void writeWrappedText(int& y, const std::string& text, TColorAttr color)
    {
        for (const auto& line : wrapText(text, size.x)) {
            if (y >= size.y)
                break;
            writeTextLine(y++, line, color);
        }
    }

    const std::vector<IpmaWarning>& warnings_;
    tvweather::Language             language_;
};

ForecastWindow::ForecastWindow(const IpmaAggregateForecast&             forecast,
                               const std::vector<IpmaWeatherType>&      weatherTypes,
                               const std::vector<IpmaWindType>&         windTypes,
                               const std::vector<IpmaWindDirectionType>& windDirectionTypes,
                               const std::vector<IpmaWarning>&          warnings,
                               std::string                              warningAreaCode,
                               tvweather::Language                      language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(2, 1, 112, 29), "", wnNoNumber),
      forecast_(forecast),
      weatherTypes_(weatherTypes),
      windTypes_(windTypes),
      windDirectionTypes_(windDirectionTypes),
      warnings_(warnings),
      warningAreaCode_(std::move(warningAreaCode)),
      language_(language)
{
    options |= ofTileable;
    updateTitle();

    TRect interior = getExtent();
    interior.grow(-1, -1);
    const auto layout = computeLayout(interior);

    dayScrollBar_ = new TScrollBar(layout.dayScrollBar);
    dayList_ = new ForecastDayListView(layout.dayList,
                                       dayScrollBar_,
                                       forecast_,
                                       weatherTypes_,
                                       windDirectionTypes_,
                                       language_);
    primaryDivider_ = new DividerView(layout.primaryDivider,
                                      layout.primaryDivider.b.x - layout.primaryDivider.a.x == 1);
    hourlyScrollBar_ = new TScrollBar(layout.hourlyScrollBar);
    hourlyView_ = new ForecastHourlyView(layout.hourlyView,
                                         hourlyScrollBar_,
                                         forecast_,
                                         weatherTypes_,
                                         windDirectionTypes_,
                                         language_);
    sideDivider_ = new DividerView(layout.sideDivider,
                                   layout.sideDivider.b.x - layout.sideDivider.a.x == 1);
    detailPane_ = new ForecastDetailPane(layout.detailPane,
                                         forecast_,
                                         weatherTypes_,
                                         windTypes_,
                                         windDirectionTypes_,
                                         language_);
    detailDivider_ = new DividerView(layout.detailDivider,
                                     layout.detailDivider.b.x - layout.detailDivider.a.x == 1);
    warningsPane_ = new ForecastWarningsPane(layout.warningsPane, warnings_, language_);

    insert(dayScrollBar_);
    insert(dayList_);
    insert(primaryDivider_);
    insert(hourlyScrollBar_);
    insert(hourlyView_);
    insert(sideDivider_);
    insert(detailPane_);
    insert(detailDivider_);
    insert(warningsPane_);

    if (!forecast_.days.empty())
        message(this, evBroadcast, cmForecastDayFocused, nullptr);
}

void ForecastWindow::changeBounds(const TRect& bounds)
{
    TWindow::changeBounds(bounds);
    relayout();
}

void ForecastWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    dayList_->setLanguage(language_);
    hourlyView_->setLanguage(language_);
    detailPane_->setLanguage(language_);
    warningsPane_->setLanguage(language_);
    drawView();
}

void ForecastWindow::sizeLimits(TPoint& min, TPoint& max)
{
    TWindow::sizeLimits(min, max);
    min.x = std::max<short>(min.x, 72);
    min.y = std::max<short>(min.y, 20);
}

void ForecastWindow::handleEvent(TEvent& event)
{
    TWindow::handleEvent(event);

    if (event.what != evCommand || event.message.command != cmRefresh)
        return;

    const std::string dayDate = dayList_ != nullptr ? dayList_->currentDayDate() : std::string();
    const std::string dateTime = hourlyView_ != nullptr ? hourlyView_->currentHourDateTime()
                                                         : std::string();

    const auto refreshed = IpmaClient::fetchAggregateForecast(forecast_.globalIdLocal,
                                                              forecast_.localName);
    if (refreshed.days.empty()) {
        messageBox(tvweather::uiStrings(language_).networkError, mfError | mfOKButton);
        clearEvent(event);
        return;
    }

    forecast_ = refreshed;

    const auto allWarnings = IpmaClient::fetchWarnings();
    warnings_.clear();
    for (const auto& warning : allWarnings) {
        if (warning.idAreaAviso == warningAreaCode_)
            warnings_.push_back(warning);
    }

    dayList_->syncData();
    hourlyView_->syncData();
    applySelectionByDate(dayDate, dateTime);
    warningsPane_->drawView();
    detailPane_->drawView();
    drawView();
    clearEvent(event);
}

void ForecastWindow::applySelectionByDate(const std::string& dayDate,
                                          const std::string& dateTime)
{
    if (dayList_ == nullptr || hourlyView_ == nullptr)
        return;

    dayList_->selectDayByDate(dayDate);
    hourlyView_->selectHourByDateTime(dateTime);
}

void ForecastWindow::relayout()
{
    TRect interior = getExtent();
    interior.grow(-1, -1);
    const auto layout = computeLayout(interior);

    dayList_->setBounds(layout.dayList);
    dayScrollBar_->setBounds(layout.dayScrollBar);
    primaryDivider_->setBounds(layout.primaryDivider);
    primaryDivider_->setVertical(layout.primaryDivider.b.x - layout.primaryDivider.a.x == 1);
    hourlyView_->setBounds(layout.hourlyView);
    hourlyScrollBar_->setBounds(layout.hourlyScrollBar);
    sideDivider_->setBounds(layout.sideDivider);
    sideDivider_->setVertical(layout.sideDivider.b.x - layout.sideDivider.a.x == 1);
    detailPane_->setBounds(layout.detailPane);
    detailDivider_->setBounds(layout.detailDivider);
    detailDivider_->setVertical(layout.detailDivider.b.x - layout.detailDivider.a.x == 1);
    warningsPane_->setBounds(layout.warningsPane);
}

void ForecastWindow::updateTitle()
{
    delete[] const_cast<char*>(title);
    title = newStr(tvweather::forecastWindowTitle(forecast_.localName, language_).c_str());
}
