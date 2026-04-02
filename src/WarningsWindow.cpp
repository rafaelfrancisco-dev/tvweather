#define Uses_TWindow
#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TScrollBar
#define Uses_TScroller
#define Uses_TKeys
#include <tvision/tv.h>

#include "tvweather/WarningsWindow.h"
#include "tvweather/Localization.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{

std::string toUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// ── WarningsView ──────────────────────────────────────────────────────────────
// TScroller subclass that renders active (non-green) weather warnings.
// Each warning block: [LEVEL] TypeName (Area), time range, blank line.

class WarningsView : public TScroller {
public:
    WarningsView(const TRect& bounds,
                 TScrollBar*  vScrollBar,
                 const std::vector<IpmaWarning>& warnings,
                 tvweather::Language             language)
        : TScroller(bounds, nullptr, vScrollBar),
          warnings_(warnings),
          language_(language)
    {
        // 3 lines per warning; at least 1 for the "no warnings" message
        const int total = warnings_.empty()
            ? 1
            : static_cast<int>(warnings_.size()) * 3;
        setLimit(0, total);
    }

    void setLanguage(tvweather::Language language)
    {
        language_ = language;
        drawView();
    }

    void draw() override
    {
        const auto& strings = tvweather::uiStrings(language_);
        TColorAttr color = getColor(1);
        TDrawBuffer b;
        int y = 0;

        if (warnings_.empty()) {
            b.moveChar(0, ' ', color, (ushort)size.x);
            b.moveStr(1, strings.warningsEmpty, color, (ushort)(size.x - 2));
            writeLine(0, y++, size.x, 1, b);
        } else {
            const int startLine = delta.y;
            const int endLine   = startLine + size.y;
            int line = 0;

            for (const auto& w : warnings_) {
                // Line 0: [LEVEL] TypeName (Area)
                if (line >= startLine && y < size.y) {
                    b.moveChar(0, ' ', color, (ushort)size.x);
                    const std::string s = "[" + toUpper(w.awarenessLevelID) + "] "
                        + w.awarenessTypeName
                        + " (" + w.idAreaAviso + ")";
                    b.moveStr(1, s.c_str(), color, (ushort)(size.x - 2));
                    writeLine(0, y++, size.x, 1, b);
                }
                ++line;
                if (line >= endLine) break;

                // Line 1: time range
                if (line >= startLine && y < size.y) {
                    b.moveChar(0, ' ', color, (ushort)size.x);
                    const std::string t =
                        std::string("  ") + strings.warningsFrom + " " + w.startTime
                        + "  " + strings.warningsTo + " " + w.endTime;
                    b.moveStr(1, t.c_str(), color, (ushort)(size.x - 2));
                    writeLine(0, y++, size.x, 1, b);
                }
                ++line;
                if (line >= endLine) break;

                // Line 2: blank separator
                if (line >= startLine && y < size.y) {
                    b.moveChar(0, ' ', color, (ushort)size.x);
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
    std::vector<IpmaWarning> warnings_;
    tvweather::Language      language_;
};

} // namespace

// ── WarningsWindow ────────────────────────────────────────────────────────────

WarningsWindow::WarningsWindow(const std::vector<IpmaWarning>& warnings,
                               tvweather::Language             language)
    : TWindowInit(TWindow::initFrame),
      TWindow(TRect(3, 2, 77, 22), "", wnNoNumber),
      warnings_(warnings),
      language_(language)
{
    options |= ofTileable;
    updateTitle();

    auto* sb = new TScrollBar(TRect(size.x - 1, 1, size.x, size.y - 1));
    insert(sb);

    TRect r = getExtent();
    r.grow(-1, -1);
    r.b.x -= 1; // room for scrollbar
    insert(new WarningsView(r, sb, warnings_, language_));
}

void WarningsWindow::setLanguage(tvweather::Language language)
{
    language_ = language;
    updateTitle();

    for (TView* view = first(); view != nullptr; view = view->nextView()) {
        if (auto* warningsView = dynamic_cast<WarningsView*>(view)) {
            warningsView->setLanguage(language_);
            break;
        }
        if (view == last)
            break;
    }

    drawView();
}

void WarningsWindow::updateTitle()
{
    delete[] const_cast<char*>(title);
    title = newStr(tvweather::uiStrings(language_).warningsTitle);
}
