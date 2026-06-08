#include "Theme.hpp"

#include <QApplication>
#include <QFont>
#include <QSettings>

namespace Acheron {
namespace UI {

bool ThemeColors::operator==(const ThemeColors &o) const
{
    return background == o.background && text == o.text && base == o.base
            && accent == o.accent && button == o.button && border == o.border
            && fontFamily == o.fontFamily && fontSize == o.fontSize;
}

namespace {

// Add (or subtract) a fixed amount to each RGB channel, clamped to [0,255].
// Predictable, unlike QColor::lighter() which scales in HSV value.
QColor adjust(const QColor &c, int delta)
{
    auto clamp = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
    return QColor(clamp(c.red() + delta), clamp(c.green() + delta), clamp(c.blue() + delta));
}

// Linear blend between two colors, t in [0,1].
QColor blend(const QColor &a, const QColor &b, double t)
{
    return QColor(static_cast<int>(a.red() + (b.red() - a.red()) * t),
                  static_cast<int>(a.green() + (b.green() - a.green()) * t),
                  static_cast<int>(a.blue() + (b.blue() - a.blue()) * t));
}

// Pick black or white text for readable contrast against bg.
QColor contrastText(const QColor &bg)
{
    double lum = 0.299 * bg.red() + 0.587 * bg.green() + 0.114 * bg.blue();
    return lum > 140 ? QColor(20, 18, 28) : QColor(245, 244, 255);
}

QString hex(const QColor &c)
{
    return c.name(QColor::HexRgb); // "#rrggbb"
}

} // namespace

ThemeColors Theme::defaults()
{
    ThemeColors c;
    c.background = QColor(0x18, 0x16, 0x22); // #181622
    c.text       = QColor(0xd2, 0xd0, 0xe1); // #d2d0e1
    c.base       = QColor(0x1e, 0x1c, 0x2c); // #1e1c2c
    c.accent     = QColor(0x7c, 0x5c, 0xc0); // #7c5cc0
    c.button     = QColor(0x28, 0x26, 0x3c); // #28263c
    c.border     = QColor(0x3a, 0x37, 0x58); // #3a3758
    c.fontFamily = QString();
    c.fontSize   = 0;
    return c;
}

ThemeColors Theme::load()
{
    ThemeColors d = defaults();
    QSettings s;
    auto col = [&](const QString &key, const QColor &fallback) {
        QString v = s.value(key).toString();
        QColor c(v);
        return c.isValid() ? c : fallback;
    };
    d.background = col("appearance/color_background", d.background);
    d.text       = col("appearance/color_text", d.text);
    d.base       = col("appearance/color_base", d.base);
    d.accent     = col("appearance/color_accent", d.accent);
    d.button     = col("appearance/color_button", d.button);
    d.border     = col("appearance/color_border", d.border);
    d.fontFamily = s.value("appearance/font_family", d.fontFamily).toString();
    d.fontSize   = s.value("appearance/font_size", d.fontSize).toInt();
    return d;
}

void Theme::save(const ThemeColors &c)
{
    QSettings s;
    s.setValue("appearance/color_background", hex(c.background));
    s.setValue("appearance/color_text", hex(c.text));
    s.setValue("appearance/color_base", hex(c.base));
    s.setValue("appearance/color_accent", hex(c.accent));
    s.setValue("appearance/color_button", hex(c.button));
    s.setValue("appearance/color_border", hex(c.border));
    s.setValue("appearance/font_family", c.fontFamily);
    s.setValue("appearance/font_size", c.fontSize);
}

QPalette Theme::buildPalette(const ThemeColors &c)
{
    QPalette p;
    const QColor disabledText = blend(c.text, c.background, 0.55);

    p.setColor(QPalette::Window, c.background);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.base);
    p.setColor(QPalette::AlternateBase, adjust(c.base, 8));
    p.setColor(QPalette::ToolTipBase, c.button);
    p.setColor(QPalette::ToolTipText, c.text);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.button);
    p.setColor(QPalette::ButtonText, c.text);
    p.setColor(QPalette::BrightText, adjust(c.accent, 45));
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, contrastText(c.accent));

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabledText);
    return p;
}

QString Theme::buildStyleSheet(const ThemeColors &c)
{
    const QString bg          = hex(c.background);
    const QString text        = hex(c.text);
    const QString base        = hex(c.base);
    const QString accent      = hex(c.accent);
    const QString button      = hex(c.button);
    const QString border      = hex(c.border);
    const QString btnHover    = hex(adjust(c.button, 10));
    const QString btnPressed  = hex(adjust(c.button, 18));
    const QString btnDisabled = hex(adjust(c.button, -6));
    const QString borderHover = hex(adjust(c.accent, -13));
    const QString borderDim   = hex(adjust(c.border, -14));
    const QString disabledTxt = hex(blend(c.text, c.background, 0.55));
    const QString selText     = hex(contrastText(c.accent));
    const QString popupBg     = hex(adjust(c.background, 11));
    const QString scrollHover = hex(adjust(c.border, 30));
    const QString tipText     = hex(adjust(c.text, 12));

    QString s;
    s += "QWidget {";
    s += "  background-color: " + bg + ";";
    s += "  color: " + text + ";";
    s += "  selection-background-color: " + accent + ";";
    s += "  selection-color: " + selText + ";";
    s += "}";

    s += "QPushButton {";
    s += "  background-color: " + button + ";";
    s += "  border: 1px solid " + border + ";";
    s += "  border-radius: 4px; padding: 6px 12px;";
    s += "}";
    s += "QPushButton:hover { background-color: " + btnHover + "; border-color: " + borderHover + "; }";
    s += "QPushButton:pressed { background-color: " + btnPressed + "; }";
    s += "QPushButton:disabled { background-color: " + btnDisabled + "; border-color: " + borderDim
            + "; color: " + disabledTxt + "; }";

    s += "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {";
    s += "  background-color: " + base + ";";
    s += "  border: 1px solid " + border + ";";
    s += "  border-radius: 4px; padding: 5px;";
    s += "}";
    s += "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, "
         "QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color: " + accent + "; }";

    s += "QComboBox::drop-down { border: none; width: 20px; }";
    s += "QComboBox QAbstractItemView { background-color: " + popupBg + "; border: 1px solid " + border
            + "; selection-background-color: " + accent + "; }";

    s += "QCheckBox::indicator, QRadioButton::indicator {";
    s += "  width: 14px; height: 14px; border: 1px solid " + border + "; background-color: " + base + ";";
    s += "}";
    s += "QCheckBox::indicator:checked, QRadioButton::indicator:checked { background-color: " + accent
            + "; border-color: " + accent + "; }";

    s += "QScrollBar:vertical, QScrollBar:horizontal { background-color: " + bg + "; border: none; margin: 0px; }";
    s += "QScrollBar::handle { background-color: " + border + "; border-radius: 4px; }";
    s += "QScrollBar::handle:hover { background-color: " + scrollHover + "; }";
    s += "QScrollBar::add-line, QScrollBar::sub-line { height: 0px; width: 0px; }";

    s += "QToolTip { background-color: " + button + "; color: " + tipText + "; border: 1px solid "
            + borderHover + "; padding: 4px; }";

    return s;
}

void Theme::apply(const ThemeColors &c)
{
    if (!qApp)
        return;

    // Remember the application's original font the first time we apply, so we
    // can restore it when the user turns custom fonts back off.
    static QFont s_defaultFont;
    static bool s_haveDefault = false;
    if (!s_haveDefault) {
        s_defaultFont = qApp->font();
        s_haveDefault = true;
    }

    qApp->setPalette(buildPalette(c));
    qApp->setStyleSheet(buildStyleSheet(c));

    if (!c.fontFamily.isEmpty()) {
        QFont f(c.fontFamily);
        f.setPointSize(c.fontSize > 0 ? c.fontSize : s_defaultFont.pointSize());
        qApp->setFont(f);
    } else {
        // No custom font: restore the original application font.
        qApp->setFont(s_defaultFont);
    }
}

} // namespace UI
} // namespace Acheron

