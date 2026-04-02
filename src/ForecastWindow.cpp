#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TKeys
#include <tvision/tv.h>

#include "tvweather/ForecastWindow.h"
#include <cstdio>
#include <string>
#include <vector>

// ── ForecastView ─────────────────────────────────────────────────────────────
// Custom TView that renders a 5-day forecast table.
// Receives data by value — no network calls inside.

namespace
{

std::string descForType(const std::vector<IpmaWeatherType>& types, int id)
{
    for (const auto& t : types)
        if (t.id == id)
            return t.descEN.empty() ? t.descPT : t.descEN;
    return "-";
}

class ForecastView : public TView {
public:
    ForecastView(const TRect& bounds,
                 const IpmaForecast& forecast,
                 const std::vector<IpmaWeatherType>& weatherTypes)
        : TView(bounds),
          forecast_(forecast),
          weatherTypes_(weatherTypes)
    {}

    void draw() override
    {
        TColorAttr color = getColor(1);
        TDrawBuffer b;

        // ── Header row ──────────────────────────────────────────────────────
        b.moveChar(0, ' ', color, (ushort)size.x);
        b.moveStr(0, " Date         Min  Max  Rain  Wind     Conditions",
                  color, (ushort)(size.x - 1));
        writeLine(0, 0, size.x, 1, b);

        // ── Separator ───────────────────────────────────────────────────────
        b.moveChar(0, '-', color, (ushort)size.x);
        writeLine(0, 1, size.x, 1, b);

        // ── Data rows ───────────────────────────────────────────────────────
        int y = 2;
        for (const auto& day : forecast_.days) {
            if (y >= size.y) break;

            b.moveChar(0, ' ', color, (ushort)size.x);

            // Date  (col 1, width 11)
            b.moveStr(1, day.forecastDate.c_str(), color, 11);

            // Min / Max  (cols 13-21)
            char tmp[16];
            std::snprintf(tmp, sizeof(tmp), "%3d", day.tMin);
            b.moveStr(13, tmp, color, 3);
            b.moveStr(16, "\xF8", color, 1); // degree symbol (CP437)
            std::snprintf(tmp, sizeof(tmp), "%3d", day.tMax);
            b.moveStr(18, tmp, color, 3);
            b.moveStr(21, "\xF8", color, 1);

            // Rain probability  (col 24, width 4)
            std::snprintf(tmp, sizeof(tmp), "%3d%%", day.precipitaProb);
            b.moveStr(23, tmp, color, 4);

            // Wind direction + speed class  (col 29)
            const std::string wind = day.predWindDir
                + " " + std::to_string(day.classWindSpeed);
            b.moveStr(28, wind.c_str(), color, 9);

            // Conditions  (col 38, remaining width)
            const std::string cond =
                descForType(weatherTypes_, day.idWeatherType);
            b.moveStr(38, cond.c_str(), color,
                      (ushort)std::max(0, size.x - 39));

            writeLine(0, y++, size.x, 1, b);
        }

        // Fill empty rows below data
        b.moveChar(0, ' ', color, (ushort)size.x);
        while (y < size.y)
            writeLine(0, y++, size.x, 1, b);
    }

private:
    IpmaForecast                  forecast_;
    std::vector<IpmaWeatherType>  weatherTypes_;
};

} // namespace

// ── ForecastWindow ────────────────────────────────────────────────────────────

ForecastWindow::ForecastWindow(const IpmaForecast&                forecast,
                               const std::vector<IpmaWeatherType>& weatherTypes)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(2, 1, 78, 14), (forecast.localName + " - 5-Day Forecast").c_str(), wnNoNumber)
{
    options |= ofTileable;

    TRect r = getExtent();
    r.grow(-1, -1);
    insert(new ForecastView(r, forecast, weatherTypes));
}

