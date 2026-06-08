#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

namespace Acheron {
namespace UI {

// User-customizable appearance settings. A small set of "anchor" colors plus
// a UI font; all the lighter/darker shades used in the stylesheet are derived
// from these so the user only has to pick a handful of values.
struct ThemeColors
{
    QColor background; // main window background  (QPalette::Window)
    QColor text;       // primary text            (QPalette::WindowText / Text)
    QColor base;       // inputs / list backgrounds (QPalette::Base)
    QColor accent;     // selection / focus / checked (QPalette::Highlight)
    QColor button;     // button background       (QPalette::Button)
    QColor border;     // borders / separators

    QString fontFamily; // empty => leave system default
    int fontSize = 0;   // <=0   => leave system default

    bool operator==(const ThemeColors &o) const;
    bool operator!=(const ThemeColors &o) const { return !(*this == o); }
};

namespace Theme {

// The built-in dark-purple defaults (matches the original hardcoded look).
ThemeColors defaults();

// Load saved colors from QSettings, falling back to defaults per-field.
ThemeColors load();

// Persist to QSettings.
void save(const ThemeColors &c);

// Build a QPalette / global stylesheet from the colors.
QPalette buildPalette(const ThemeColors &c);
QString buildStyleSheet(const ThemeColors &c);

// Apply palette + stylesheet + font to the running application immediately.
void apply(const ThemeColors &c);

} // namespace Theme

} // namespace UI
} // namespace Acheron
