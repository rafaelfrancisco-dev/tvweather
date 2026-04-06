#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#include <tvision/tv.h>

#include "tvweather/ObservationsWindow.h"
#include "tvweather/Localization.h"
#include <cstdio>
#include <string>

namespace {

constexpr char cpWindowPane[] = "\x06";

class ObservationsPaneView : public TView {
public:
    ObservationsPaneView(const TRect&                  bounds,
                         const IpmaStationObservation& observation,
                         double                        stationDistanceKm,
                         tvweather::Language           language) noexcept
        : TView(bounds),
          observation_(observation),
          stationDistanceKm_(stationDistanceKm),
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

        // Fill background.
        TDrawBuffer buf;
        buf.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        for (int y = 0; y < size.y; ++y)
            writeLine(0, y, size.x, 1, buf);

        if (observation_.timestamp.empty()) {
            writeTextLine(0, strings.obsEmpty, color);
            return;
        }

        char distBuf[32];
        std::snprintf(distBuf, sizeof(distBuf), "%.1f km", stationDistanceKm_);

        int y = 0;
        writeTextLine(y++, std::string(strings.obsStationLabel) + ": "
                          + observation_.localEstacao
                          + " (" + distBuf + ")", color);
        writeTextLine(y++, std::string(strings.obsObservedAt) + ": "
                          + observation_.timestamp, color);
        ++y;

        if (observation_.temperatura.has_value()) {
            char val[16];
            std::snprintf(val, sizeof(val), "%.1f\xF8", static_cast<double>(*observation_.temperatura));
            writeTextLine(y++, std::string(strings.forecastTemperature) + ": " + val, color);
        }

        if (observation_.intensidadeVentoKM.has_value()) {
            std::string windLine = std::string(strings.forecastWindSpeed) + ": ";
            char val[16];
            std::snprintf(val, sizeof(val), "%.0f km/h", static_cast<double>(*observation_.intensidadeVentoKM));
            windLine += val;
            if (observation_.idDireccVento.has_value()) {
                windLine += " ";
                windLine += tvweather::stationWindDirectionLabel(*observation_.idDireccVento);
            }
            writeTextLine(y++, windLine, color);
        }

        if (observation_.humidade.has_value()) {
            char val[16];
            std::snprintf(val, sizeof(val), "%.0f%%", static_cast<double>(*observation_.humidade));
            writeTextLine(y++, std::string(strings.forecastHumidity) + ": " + val, color);
        }

        if (observation_.pressao.has_value()) {
            char val[16];
            std::snprintf(val, sizeof(val), "%.1f hPa", static_cast<double>(*observation_.pressao));
            writeTextLine(y++, std::string(strings.obsPressure) + ": " + val, color);
        }

        if (observation_.radiacao.has_value()) {
            char val[16];
            std::snprintf(val, sizeof(val), "%.0f W/m\xFD", static_cast<double>(*observation_.radiacao));
            writeTextLine(y++, std::string(strings.obsRadiation) + ": " + val, color);
        }

        if (observation_.precAcumulada.has_value()) {
            char val[16];
            std::snprintf(val, sizeof(val), "%.1f mm", static_cast<double>(*observation_.precAcumulada));
            writeTextLine(y++, std::string(strings.obsPrecipAccum) + ": " + val, color);
        }
    }

    TPalette& getPalette() const override
    {
        static TPalette palette(cpWindowPane, sizeof(cpWindowPane) - 1);
        return palette;
    }

private:
    void writeTextLine(int y, const std::string& text, TColorAttr color)
    {
        if (y < 0 || y >= size.y)
            return;

        TDrawBuffer buf;
        buf.moveChar(0, ' ', color, static_cast<ushort>(size.x));
        buf.moveStr(1, text.c_str(), color, static_cast<ushort>(size.x - 1));
        writeLine(0, y, size.x, 1, buf);
    }

    IpmaStationObservation observation_;
    double                 stationDistanceKm_;
    tvweather::Language    language_;
};

} // namespace

// ── ObservationsWindow ────────────────────────────────────────────────────────

ObservationsWindow::ObservationsWindow(const IpmaStationObservation& observation,
                                       double                        stationDistanceKm,
                                       const std::string&            selectedLocationName,
                                       tvweather::Language           language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(5, 3, 55, 17), "", wnNoNumber),
      observation_(observation),
      stationDistanceKm_(stationDistanceKm),
      selectedLocationName_(selectedLocationName),
      language_(language)
{
    options |= ofTileable;
    updateTitle();

    TRect r = getExtent();
    r.grow(-1, -1);
    insert(new ObservationsPaneView(r, observation_, stationDistanceKm_, language_));
}

void ObservationsWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    for (TView* view = first(); view != nullptr; view = view->nextView()) {
        if (auto* pane = dynamic_cast<ObservationsPaneView*>(view)) {
            pane->setLanguage(language_);
            break;
        }
        if (view == last)
            break;
    }

    drawView();
}

void ObservationsWindow::updateTitle()
{
    const auto& strings = tvweather::uiStrings(language_);
    const std::string t = selectedLocationName_ + strings.obsWindowSuffix;
    delete[] const_cast<char*>(title);
    title = newStr(t.c_str());
}
