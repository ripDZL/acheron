#include "SettingsWindow.hpp"

#include "AppearancePage.hpp"
#include "UI/TypingIndicator.hpp"
#include "UI/Chat/ChatView.hpp"
#include "UI/TabBar/TabBar.hpp"
#include "Core/ImageManager.hpp"
#include "Core/UserManager.hpp"

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

    // ── Scrolling ────────────────────────────────────────────────────────────
    auto *scrollLabel = new QLabel(tr("Scrolling"), page);
    QFont sbf = scrollLabel->font();
    sbf.setBold(true);
    scrollLabel->setFont(sbf);
    layout->addSpacing(10);
    layout->addWidget(scrollLabel);

    invertWheelCheckbox = new QCheckBox(tr("Invert scroll wheel in chat"), page);
    layout->addWidget(invertWheelCheckbox);
    connect(invertWheelCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        ChatView::setWheelInverted(checked);
    });

    auto *scrollForm = new QFormLayout;
    scrollForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    scrollSpeedSpin = new QDoubleSpinBox(page);
    scrollSpeedSpin->setRange(0.25, 5.0);
    scrollSpeedSpin->setSingleStep(0.25);
    scrollSpeedSpin->setDecimals(2);
    scrollSpeedSpin->setSuffix(tr("x"));
    scrollSpeedSpin->setValue(ChatView::scrollSpeed());
    connect(scrollSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [](double v) { ChatView::setScrollSpeed(v); });
    scrollForm->addRow(tr("Chat scroll speed (wheel)"), scrollSpeedSpin);
    layout->addSpacing(4);
    layout->addLayout(scrollForm);

    // ── Tabs ─────────────────────────────────────────────────────────────────
    auto *tabsLabel = new QLabel(tr("Tabs"), page);
    QFont tbf = tabsLabel->font();
    tbf.setBold(true);
    tabsLabel->setFont(tbf);
    layout->addSpacing(10);
    layout->addWidget(tabsLabel);

    showCloseButtonCheckbox = new QCheckBox(tr("Show close button on tabs"), page);
    layout->addWidget(showCloseButtonCheckbox);
    connect(showCloseButtonCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        TabBar::setShowCloseButton(checked);
    });

    extraActiveHighlightCheckbox = new QCheckBox(tr("Extra highlighting on the active tab"), page);
    layout->addWidget(extraActiveHighlightCheckbox);
    connect(extraActiveHighlightCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        TabBar::setExtraActiveHighlight(checked);
    });

    avoidRedundantTabsCheckbox = new QCheckBox(tr("Avoid creating redundant tabs"), page);
    avoidRedundantTabsCheckbox->setToolTip(
            tr("Opening a channel that's already in a tab switches to it instead of duplicating."));
    layout->addWidget(avoidRedundantTabsCheckbox);
    connect(avoidRedundantTabsCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        TabBar::setAvoidRedundantTabs(checked);
    });

    auto *tabsNote = new QLabel(tr("Tab appearance changes take effect on the next tab interaction."), page);
    tabsNote->setWordWrap(true);
    tabsNote->setStyleSheet("color: palette(mid);");
    layout->addSpacing(4);
    layout->addWidget(tabsNote);

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

void SettingsWindow::buildAppearancePage()
{
    // Upstream's design-token appearance page (colors + per-role fonts).
    pages->addWidget(new AppearancePage(this));
}

void SettingsWindow::loadSettings()
{
    QSettings settings;
    inMemoryCacheCheckbox->setChecked(settings.value("general/in_memory_cache", false).toBool());
    downloadImagesCheckbox->setChecked(Core::ImageManager::networkImagesEnabled());
    invertWheelCheckbox->setChecked(ChatView::wheelInverted());
    showCloseButtonCheckbox->setChecked(TabBar::showCloseButton());
    extraActiveHighlightCheckbox->setChecked(TabBar::extraActiveHighlight());
    avoidRedundantTabsCheckbox->setChecked(TabBar::avoidRedundantTabs());
    showNicknamesCheckbox->setChecked(Core::UserManager::showNicknames());
    showTypingCheckbox->setChecked(TypingIndicator::showTyping());

    QString locale = settings.value("language/locale", "en_US").toString();
    int li = languageCombo->findData(locale);
    if (li >= 0)
        languageCombo->setCurrentIndex(li);
}

} // namespace UI
} // namespace Acheron

