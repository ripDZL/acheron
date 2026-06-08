#include "SettingsWindow.hpp"

#include "UI/Theme.hpp"
#include "UI/TypingIndicator.hpp"
#include "Core/ImageManager.hpp"
#include "Core/UserManager.hpp"

#include <QColorDialog>
#include <QFontDatabase>
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
    categoryList->addItem(tr("Discord"));
    categoryList->addItem(tr("Style"));
    categoryList->addItem(tr("Language"));
    categoryList->addItem(tr("Audio"));
    categoryList->setCurrentRow(0);

    pages = new QStackedWidget(this);

    // Pages are added in the same order as the category list.
    buildGeneralPage();
    buildDiscordPage();
    buildAppearancePage();
    buildLanguagePage();
    buildAudioPage();

    connect(categoryList, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);

    mainLayout->addWidget(categoryList);
    mainLayout->addWidget(pages, 1);
}

void SettingsWindow::buildGeneralPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    inMemoryCacheCheckbox = new QCheckBox(tr("In-memory cache database (requires restart)"), page);
    layout->addWidget(inMemoryCacheCheckbox);
    connect(inMemoryCacheCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        QSettings().setValue("general/in_memory_cache", checked);
    });

    downloadImagesCheckbox = new QCheckBox(tr("Download images and avatars from the network"), page);
    downloadImagesCheckbox->setToolTip(
            tr("Uncheck to save bandwidth. Cached images still display; new ones won't be fetched."));
    layout->addWidget(downloadImagesCheckbox);
    connect(downloadImagesCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        Core::ImageManager::setNetworkImagesEnabled(checked);
    });

    auto *spacingForm = new QFormLayout;
    spacingForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    messageSpacingSpin = new QSpinBox(page);
    messageSpacingSpin->setRange(0, 40);
    messageSpacingSpin->setSuffix(tr(" px"));
    messageSpacingSpin->setValue(QSettings().value("general/message_spacing", 0).toInt());
    connect(messageSpacingSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
        QSettings().setValue("general/message_spacing", v);
    });
    spacingForm->addRow(tr("Extra message spacing"), messageSpacingSpin);
    layout->addSpacing(8);
    layout->addLayout(spacingForm);

    auto *genNote = new QLabel(tr("Message spacing applies after restarting acheron."), page);
    genNote->setWordWrap(true);
    genNote->setStyleSheet("color: palette(mid);");
    layout->addWidget(genNote);

    layout->addStretch();
    pages->addWidget(page);
}

void SettingsWindow::buildDiscordPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *displayLabel = new QLabel(tr("Display Features"), page);
    QFont bf = displayLabel->font();
    bf.setBold(true);
    displayLabel->setFont(bf);
    layout->addWidget(displayLabel);

    showNicknamesCheckbox = new QCheckBox(tr("Show nicknames in chat and member list"), page);
    showNicknamesCheckbox->setToolTip(tr("When off, shows usernames instead of per-server nicknames."));
    layout->addWidget(showNicknamesCheckbox);
    connect(showNicknamesCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        Core::UserManager::setShowNicknames(checked);
    });

    showTypingCheckbox = new QCheckBox(tr("Show when users are typing"), page);
    layout->addWidget(showTypingCheckbox);
    connect(showTypingCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        TypingIndicator::setShowTyping(checked);
    });

    // Sizing controls (applied on restart)
    auto *sizeForm = new QFormLayout;
    sizeForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    avatarSizeSpin = new QSpinBox(page);
    avatarSizeSpin->setRange(16, 128);
    avatarSizeSpin->setSuffix(tr(" px"));
    avatarSizeSpin->setValue(QSettings().value("discord/avatar_size", 32).toInt());
    connect(avatarSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
        QSettings().setValue("discord/avatar_size", v);
    });
    sizeForm->addRow(tr("Chat avatar size"), avatarSizeSpin);

    emojiSizeSpin = new QSpinBox(page);
    emojiSizeSpin->setRange(12, 64);
    emojiSizeSpin->setSuffix(tr(" px"));
    emojiSizeSpin->setValue(QSettings().value("discord/emoji_size", 22).toInt());
    connect(emojiSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
        QSettings().setValue("discord/emoji_size", v);
    });
    sizeForm->addRow(tr("Chat emoji size"), emojiSizeSpin);

    layout->addSpacing(8);
    layout->addLayout(sizeForm);

    auto *note = new QLabel(
            tr("Nickname changes apply to newly displayed messages (switch channels to refresh). "
               "Avatar and emoji size changes apply after restarting acheron."),
            page);
    note->setWordWrap(true);
    note->setStyleSheet("color: palette(mid);");
    layout->addSpacing(8);
    layout->addWidget(note);

    layout->addStretch();
    pages->addWidget(page);
}

void SettingsWindow::buildLanguagePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *row = new QHBoxLayout;
    row->addWidget(new QLabel(tr("Language:"), page));
    languageCombo = new QComboBox(page);
    languageCombo->addItem(tr("English (US)"), "en_US");
    languageCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    row->addWidget(languageCombo, 1);
    layout->addLayout(row);

    auto *note = new QLabel(
            tr("Additional translations aren't bundled yet. Your selection is saved and will "
               "take effect as more languages are added."),
            page);
    note->setWordWrap(true);
    note->setStyleSheet("color: palette(mid);");
    layout->addSpacing(8);
    layout->addWidget(note);

    connect(languageCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        QSettings().setValue("language/locale", languageCombo->currentData().toString());
    });

    layout->addStretch();
    pages->addWidget(page);
}

void SettingsWindow::buildAudioPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *header = new QLabel(tr("Voice & Audio"), page);
    QFont bf = header->font();
    bf.setBold(true);
    header->setFont(bf);
    layout->addWidget(header);

    auto *info = new QLabel(
            tr("Microphone and output device selection, input/output levels, voice-activity "
               "sensitivity, and Push-to-Talk are configured in the Voice panel, which opens "
               "from the voice status bar when you're connected to a voice channel.\n\n"
               "These controls live there because they require an active voice connection. "
               "Inline audio settings here are planned as a follow-up."),
            page);
    info->setWordWrap(true);
    layout->addWidget(info);

    layout->addStretch();
    pages->addWidget(page);
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
    fontCombo = new QComboBox(page);
    // Populate from the locally installed font families. Using a plain QComboBox
    // (rather than QFontComboBox) renders every entry in the normal UI font, so
    // tall/decorative fonts don't overlap or clip in the dropdown.
    fontCombo->addItems(QFontDatabase::families());
    fontCombo->setMaxVisibleItems(20);
    fontCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
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
    {
        // Select the saved family, or fall back to the current UI font family.
        QString family = hasCustomFont ? currentTheme.fontFamily
                                       : QApplication::font().family();
        int idx = fontCombo->findText(family);
        if (idx >= 0)
            fontCombo->setCurrentIndex(idx);
    }
    int defaultPt = QApplication::font().pointSize();
    if (defaultPt <= 0)
        defaultPt = 10;
    fontSizeSpin->setValue(currentTheme.fontSize > 0 ? currentTheme.fontSize : defaultPt);

    auto applyFont = [this]() {
        if (useCustomFont->isChecked()) {
            currentTheme.fontFamily = fontCombo->currentText();
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
    connect(fontCombo, &QComboBox::currentTextChanged, this, [applyFont]() { applyFont(); });
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
    downloadImagesCheckbox->setChecked(Core::ImageManager::networkImagesEnabled());
    showNicknamesCheckbox->setChecked(Core::UserManager::showNicknames());
    showTypingCheckbox->setChecked(TypingIndicator::showTyping());

    QString locale = settings.value("language/locale", "en_US").toString();
    int li = languageCombo->findData(locale);
    if (li >= 0)
        languageCombo->setCurrentIndex(li);
}

} // namespace UI
} // namespace Acheron

