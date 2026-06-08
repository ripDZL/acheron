#include "SettingsWindow.hpp"

#include "UI/Theme.hpp"

#include <QColorDialog>
#include <QSettings>

namespace Acheron {
namespace UI {

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Settings"));
    resize(620, 460);

    setupUi();
    loadSettings();
}

void SettingsWindow::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);

    categoryList = new QListWidget(this);
    categoryList->setFixedWidth(150);
    categoryList->addItem(tr("General"));
    categoryList->addItem(tr("Appearance"));
    categoryList->setCurrentRow(0);

    pages = new QStackedWidget(this);

    auto *generalPage = new QWidget(this);
    auto *generalLayout = new QVBoxLayout(generalPage);

    inMemoryCacheCheckbox = new QCheckBox(tr("In-memory cache database (requires restart)"), generalPage);
    generalLayout->addWidget(inMemoryCacheCheckbox);
    generalLayout->addStretch();

    pages->addWidget(generalPage);

    buildAppearancePage();

    connect(categoryList, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);

    connect(inMemoryCacheCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        QSettings settings;
        settings.setValue("general/in_memory_cache", checked);
    });

    mainLayout->addWidget(categoryList);
    mainLayout->addWidget(pages, 1);
}

QPushButton *SettingsWindow::makeColorSwatch(const QColor &initial)
{
    auto *btn = new QPushButton(this);
    btn->setFixedSize(60, 24);
    btn->setCursor(Qt::PointingHandCursor);
    setSwatchColor(btn, initial);
    return btn;
}

void SettingsWindow::setSwatchColor(QPushButton *btn, const QColor &c)
{
    double lum = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue();
    QString fg = lum > 140 ? "#141414" : "#f0f0f0";
    // Inline style overrides the global QPushButton theme so the swatch shows
    // exactly the chosen color.
    btn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; font-size: 9px; "
                               "border: 1px solid #888; border-radius: 3px; } "
                               "QPushButton:hover { border: 1px solid #fff; }")
                               .arg(c.name(QColor::HexRgb), fg));
    btn->setText(c.name(QColor::HexRgb).toUpper());
}

void SettingsWindow::buildAppearancePage()
{
    currentTheme = Theme::load();

    auto *page = new QWidget(this);
    auto *outer = new QVBoxLayout(page);

    auto *colorsLabel = new QLabel(tr("Colors"), page);
    QFont hf = colorsLabel->font();
    hf.setBold(true);
    colorsLabel->setFont(hf);
    outer->addWidget(colorsLabel);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    struct Row {
        const char *label;
        QPushButton **swatch;
        QColor *value;
    };
    // clang-format off
    Row rows[] = {
        { QT_TR_NOOP("Background"),       &bgSwatch,     &currentTheme.background },
        { QT_TR_NOOP("Text"),             &textSwatch,   &currentTheme.text       },
        { QT_TR_NOOP("Input background"), &baseSwatch,   &currentTheme.base       },
        { QT_TR_NOOP("Accent"),          &accentSwatch, &currentTheme.accent     },
        { QT_TR_NOOP("Button"),          &buttonSwatch, &currentTheme.button     },
        { QT_TR_NOOP("Border"),          &borderSwatch, &currentTheme.border     },
    };
    // clang-format on

    for (auto &r : rows) {
        QPushButton *sw = makeColorSwatch(*r.value);
        *r.swatch = sw;
        QColor *target = r.value;
        connect(sw, &QPushButton::clicked, this, [this, sw, target]() {
            QColor chosen = QColorDialog::getColor(*target, this, tr("Pick a color"));
            if (chosen.isValid()) {
                *target = chosen;
                setSwatchColor(sw, chosen);
                applyAndSaveTheme();
            }
        });
        form->addRow(tr(r.label), sw);
    }
    outer->addLayout(form);

    outer->addSpacing(12);
    auto *fontLabel = new QLabel(tr("Font"), page);
    fontLabel->setFont(hf);
    outer->addWidget(fontLabel);

    useCustomFont = new QCheckBox(tr("Use a custom UI font"), page);
    outer->addWidget(useCustomFont);

    auto *fontRow = new QHBoxLayout;
    fontCombo = new QFontComboBox(page);
    fontSizeSpin = new QSpinBox(page);
    fontSizeSpin->setRange(7, 28);
    fontSizeSpin->setSuffix(tr(" pt"));
    fontRow->addWidget(fontCombo, 1);
    fontRow->addWidget(fontSizeSpin);
    outer->addLayout(fontRow);

    bool hasCustomFont = !currentTheme.fontFamily.isEmpty();
    useCustomFont->setChecked(hasCustomFont);
    fontCombo->setEnabled(hasCustomFont);
    fontSizeSpin->setEnabled(hasCustomFont);
    if (hasCustomFont)
        fontCombo->setCurrentFont(QFont(currentTheme.fontFamily));
    int defaultPt = QApplication::font().pointSize();
    if (defaultPt <= 0)
        defaultPt = 10;
    fontSizeSpin->setValue(currentTheme.fontSize > 0 ? currentTheme.fontSize : defaultPt);

    auto applyFont = [this]() {
        if (useCustomFont->isChecked()) {
            currentTheme.fontFamily = fontCombo->currentFont().family();
            currentTheme.fontSize = fontSizeSpin->value();
        } else {
            currentTheme.fontFamily = QString();
            currentTheme.fontSize = 0;
        }
        applyAndSaveTheme();
    };
    connect(useCustomFont, &QCheckBox::toggled, this, [this, applyFont](bool on) {
        fontCombo->setEnabled(on);
        fontSizeSpin->setEnabled(on);
        applyFont();
    });
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [applyFont]() { applyFont(); });
    connect(fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [applyFont]() { applyFont(); });

    outer->addSpacing(12);
    auto *note = new QLabel(
            tr("Changes apply immediately. A few areas with their own styling may need a restart "
               "to fully match. Font changes apply best after restart."),
            page);
    note->setWordWrap(true);
    note->setStyleSheet("color: palette(mid);");
    outer->addWidget(note);

    outer->addStretch();

    auto *resetBtn = new QPushButton(tr("Reset to Defaults"), page);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        currentTheme = Theme::defaults();
        setSwatchColor(bgSwatch, currentTheme.background);
        setSwatchColor(textSwatch, currentTheme.text);
        setSwatchColor(baseSwatch, currentTheme.base);
        setSwatchColor(accentSwatch, currentTheme.accent);
        setSwatchColor(buttonSwatch, currentTheme.button);
        setSwatchColor(borderSwatch, currentTheme.border);
        useCustomFont->setChecked(false);
        applyAndSaveTheme();
    });
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(resetBtn);
    outer->addLayout(btnRow);

    pages->addWidget(page);
}

void SettingsWindow::applyAndSaveTheme()
{
    Theme::save(currentTheme);
    Theme::apply(currentTheme);
}

void SettingsWindow::loadSettings()
{
    QSettings settings;
    inMemoryCacheCheckbox->setChecked(settings.value("general/in_memory_cache", false).toBool());
}

} // namespace UI
} // namespace Acheron
