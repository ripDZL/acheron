#include "Core/SoundManager.hpp"
#include "SettingsWindow.hpp"
#include "Core/Qt5Compat.hpp"

#include "AppearancePage.hpp"
#include "UI/TypingIndicator.hpp"
#include "UI/Chat/ChatView.hpp"
#include "UI/Input/MessageInput.hpp"
#include "UI/MemberList/MemberListModel.hpp"
#include "UI/TabBar/TabBar.hpp"
#include "UI/ChannelList/ChannelFilterProxyModel.hpp"
#include "Core/ImageManager.hpp"
#include "Core/UserManager.hpp"
#include "Core/AV/AudioDevicePrefs.hpp"
#ifndef ACHERON_NO_VOICE
#include "Core/AV/IAudioBackend.hpp"
#endif

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
    // Refresh the device lists each time the Audio page (row 4) is opened.
    connect(categoryList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row == 4)
            populateAudioDevices();
    });

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

    // ── Message input ─────────────────────────────────────────────────────────
    auto *inputLabel = new QLabel(tr("Message Input"), page);
    QFont inf = inputLabel->font();
    inf.setBold(true);
    inputLabel->setFont(inf);
    layout->addSpacing(10);
    layout->addWidget(inputLabel);

    charCounterCheckbox = new QCheckBox(tr("Show character counter in the message box"), page);
    charCounterCheckbox->setChecked(MessageInput::counterEnabled());
    layout->addWidget(charCounterCheckbox);

    charCounterColorCheckbox = new QCheckBox(
            tr("Colour the counter as you approach the limit"), page);
    charCounterColorCheckbox->setChecked(MessageInput::counterColorEffects());
    charCounterColorCheckbox->setEnabled(MessageInput::counterEnabled());
    layout->addWidget(charCounterColorCheckbox);

    connect(charCounterCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        MessageInput::setCounterEnabled(checked);
        if (charCounterColorCheckbox)
            charCounterColorCheckbox->setEnabled(checked);
    });
    connect(charCounterColorCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        MessageInput::setCounterColorEffects(checked);
    });

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

    restorePreviousTabsCheckbox = new QCheckBox(tr("Restore previous tabs on launch"), page);
    restorePreviousTabsCheckbox->setToolTip(
            tr("Reopen the tabs you had open when you last closed Acheron. Each tab becomes active "
               "once its account finishes connecting."));
    layout->addWidget(restorePreviousTabsCheckbox);
    connect(restorePreviousTabsCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        TabBar::setRestorePreviousSession(checked);
    });

    showAccountsOnLaunchCheckbox = new QCheckBox(tr("Always show the Accounts panel on launch"), page);
    layout->addWidget(showAccountsOnLaunchCheckbox);
    connect(showAccountsOnLaunchCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        QSettings().setValue("general/show_accounts_on_launch", checked);
    });

    auto *tabsNote = new QLabel(tr("Tab appearance changes take effect on the next tab interaction."), page);
    tabsNote->setWordWrap(true);
    tabsNote->setStyleSheet("color: palette(mid);");
    layout->addSpacing(4);
    layout->addWidget(tabsNote);

    // ── System Tray ──────────────────────────────────────────────────────────
    auto *trayLabel = new QLabel(tr("System Tray"), page);
    QFont trbf = trayLabel->font();
    trbf.setBold(true);
    trayLabel->setFont(trbf);
    layout->addSpacing(10);
    layout->addWidget(trayLabel);

    showTrayIconCheckbox = new QCheckBox(tr("Show icon in system tray"), page);
    layout->addWidget(showTrayIconCheckbox);
    connect(showTrayIconCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        QSettings().setValue("tray/show_icon", checked);
    });

    closeToTrayCheckbox = new QCheckBox(tr("Keep Acheron running in the tray when the window is closed"), page);
    layout->addWidget(closeToTrayCheckbox);
    connect(closeToTrayCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        QSettings().setValue("tray/close_to_tray", checked);
    });

    auto *trayNote = new QLabel(
            tr("Showing or hiding the tray icon takes effect after restarting acheron."), page);
    trayNote->setWordWrap(true);
    trayNote->setStyleSheet("color: palette(mid);");
    layout->addSpacing(4);
    layout->addWidget(trayNote);

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

    showOwnerCrownCheckbox = new QCheckBox(tr("Show owner crown in the member list"), page);
    showOwnerCrownCheckbox->setToolTip(
            tr("Marks the server owner with a crown next to their name, even on large servers."));
    layout->addWidget(showOwnerCrownCheckbox);
    connect(showOwnerCrownCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        MemberListModel::setShowOwnerCrown(checked);
    });

    showHiddenChannelsCheckbox = new QCheckBox(tr("Show hidden channels"), page);
    showHiddenChannelsCheckbox->setToolTip(
            tr("Show channels you don't have access to, locked and read-only."));
    layout->addWidget(showHiddenChannelsCheckbox);
    connect(showHiddenChannelsCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
        ChannelFilterProxyModel::setShowHiddenChannels(checked);
        emit showHiddenChannelsChanged();
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

void SettingsWindow::populateAudioDevices()
{
    if (!audioInputCombo || !audioOutputCombo)
        return;

#ifndef ACHERON_NO_VOICE
    // Create the enumeration backend once and keep it alive (parented to this
    // window) instead of spinning a fresh audio context up and down on each call.
    if (!enumBackend)
        enumBackend = Core::AV::IAudioBackend::create(this).release();
    auto &prefs = Core::AV::AudioDevicePrefs::instance();
    const QByteArray savedIn = prefs.inputId();
    const QByteArray savedOut = prefs.outputId();
    const QString savedInName = prefs.inputName();
    const QString savedOutName = prefs.outputName();

    auto fill = [](QComboBox *combo, const QList<Core::AV::AudioDeviceInfo> &devs,
                   const QByteArray &sel, const QString &selName, const QString &defaultLabel) {
        QSignalBlocker block(combo);
        combo->clear();
        combo->addItem(defaultLabel, QByteArray());
        int selIdx = 0;
        for (const auto &d : devs) {
            combo->addItem(d.description, d.id);
            // id bytes aren't stable across backend instances, so also match by name
            if ((!sel.isEmpty() && d.id == sel) || (!selName.isEmpty() && d.description == selName))
                selIdx = combo->count() - 1;
        }
        combo->setCurrentIndex(selIdx);
    };
    fill(audioInputCombo, enumBackend->availableInputDevices(), savedIn, savedInName, tr("System Default"));
    fill(audioOutputCombo, enumBackend->availableOutputDevices(), savedOut, savedOutName, tr("System Default"));
#else
    for (QComboBox *c : {audioInputCombo, audioOutputCombo}) {
        QSignalBlocker block(c);
        c->clear();
        c->addItem(tr("Voice support not built"));
        c->setEnabled(false);
    }
#endif
}

void SettingsWindow::buildAudioPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto boldLabel = [page](const QString &text) {
        auto *l = new QLabel(text, page);
        QFont f = l->font();
        f.setBold(true);
        l->setFont(f);
        return l;
    };

    // --- Devices ---
    layout->addWidget(boldLabel(tr("Devices")));
    auto *devForm = new QFormLayout;
    audioInputCombo = new QComboBox(page);
    audioOutputCombo = new QComboBox(page);
    devForm->addRow(tr("Input Device"), audioInputCombo);
    devForm->addRow(tr("Output Device"), audioOutputCombo);
    layout->addLayout(devForm);

    // --- Mixing ---
    layout->addWidget(boldLabel(tr("Mixing")));
    auto *mixForm = new QFormLayout;
    inputChannelsCombo = new QComboBox(page);
    inputChannelsCombo->addItem(tr("Stereo"));
    inputChannelsCombo->addItem(tr("Mono"));
    inputChannelsCombo->setToolTip(tr("Mono downmixes your microphone before sending (shares the Mix Mono setting)."));
    mixForm->addRow(tr("Input Channels"), inputChannelsCombo);
    layout->addLayout(mixForm);

    // --- Behavior ---
    layout->addWidget(boldLabel(tr("Behavior")));
    auto *behForm = new QFormLayout;
    micModeCombo = new QComboBox(page);
    micModeCombo->addItem(tr("Voice Activity"));
    micModeCombo->addItem(tr("Push to Talk"));
    behForm->addRow(tr("Microphone Mode"), micModeCombo);

    auto *pttRow = new QHBoxLayout;
    pttHotkeyEdit = new QKeySequenceEdit(page);
    ACHERON_SET_MAX_SEQ_LEN(pttHotkeyEdit, 1);
    pttHotkeyEdit->setToolTip(tr("Click here, then press the key to use for Push to Talk."));
    pttHotkeyClear = new QPushButton(tr("Clear"), page);
    pttRow->addWidget(pttHotkeyEdit, 1);
    pttRow->addWidget(pttHotkeyClear);
    behForm->addRow(tr("Push to Talk Hotkey"), pttRow);
    layout->addLayout(behForm);

    // --- Notifications ---
    layout->addWidget(boldLabel(tr("Notifications")));
    notificationSoundsCheckbox = new QCheckBox(
            tr("Play sounds when users join or leave your voice channel, and when the "
               "connection to Discord is lost"), page);
    notificationSoundsCheckbox->setChecked(Core::SoundManager::soundsEnabled());
    connect(notificationSoundsCheckbox, &QCheckBox::toggled, this, [](bool checked) {
        Core::SoundManager::setSoundsEnabled(checked);
    });
    layout->addWidget(notificationSoundsCheckbox);

    auto *note = new QLabel(
            tr("Device and channel changes apply the next time you connect to voice. "
               "Microphone mode and hotkey apply when the voice panel is open."),
            page);
    note->setWordWrap(true);
    note->setStyleSheet("color: palette(mid);");
    layout->addWidget(note);

    // --- Load current values (before connecting handlers so we don't echo writes) ---
    populateAudioDevices();
    {
        QSettings s;
        inputChannelsCombo->setCurrentIndex(s.value("voice/mix_mono", false).toBool() ? 1 : 0);
        const bool ptt = s.value("voice/ptt_mode", false).toBool();
        micModeCombo->setCurrentIndex(ptt ? 1 : 0);
        const int key = s.value("voice/ptt_key", 0).toInt();
        if (key != 0)
            pttHotkeyEdit->setKeySequence(QKeySequence(key));
        pttHotkeyEdit->setEnabled(ptt);
        pttHotkeyClear->setEnabled(ptt && key != 0);
    }

    // --- Handlers (write QSettings; these keys are the shared source of truth) ---
    connect(audioInputCombo, &QComboBox::activated, this, [this](int i) {
        Core::AV::AudioDevicePrefs::instance().setInput(audioInputCombo->itemData(i).toByteArray(),
                                                        audioInputCombo->itemText(i));
    });
    connect(audioOutputCombo, &QComboBox::activated, this, [this](int i) {
        Core::AV::AudioDevicePrefs::instance().setOutput(audioOutputCombo->itemData(i).toByteArray(),
                                                         audioOutputCombo->itemText(i));
    });
    connect(inputChannelsCombo, &QComboBox::currentIndexChanged, this, [this](int i) {
        QSettings().setValue("voice/mix_mono", i == 1);
    });
    connect(micModeCombo, &QComboBox::currentIndexChanged, this, [this](int i) {
        const bool ptt = (i == 1);
        QSettings().setValue("voice/ptt_mode", ptt);
        pttHotkeyEdit->setEnabled(ptt);
        pttHotkeyClear->setEnabled(ptt && !pttHotkeyEdit->keySequence().isEmpty());
    });
    connect(pttHotkeyEdit, &QKeySequenceEdit::keySequenceChanged, this, [this](const QKeySequence &seq) {
        const int key = seq.isEmpty() ? 0 : ACHERON_KEY_INT(seq, 0);
        QSettings().setValue("voice/ptt_key", key);
        pttHotkeyClear->setEnabled(micModeCombo->currentIndex() == 1 && key != 0);
    });
    connect(pttHotkeyClear, &QPushButton::clicked, this, [this]() {
        pttHotkeyEdit->clear();
        QSettings().setValue("voice/ptt_key", 0);
        pttHotkeyClear->setEnabled(false);
    });

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
    restorePreviousTabsCheckbox->setChecked(TabBar::restorePreviousSession());
    showAccountsOnLaunchCheckbox->setChecked(settings.value("general/show_accounts_on_launch", false).toBool());
    showTrayIconCheckbox->setChecked(settings.value("tray/show_icon", true).toBool());
    closeToTrayCheckbox->setChecked(settings.value("tray/close_to_tray", true).toBool());
    showNicknamesCheckbox->setChecked(Core::UserManager::showNicknames());
    showTypingCheckbox->setChecked(TypingIndicator::showTyping());
    showOwnerCrownCheckbox->setChecked(MemberListModel::showOwnerCrown());
    showHiddenChannelsCheckbox->setChecked(ChannelFilterProxyModel::showHiddenChannels());

    QString locale = settings.value("language/locale", "en_US").toString();
    int li = languageCombo->findData(locale);
    if (li >= 0)
        languageCombo->setCurrentIndex(li);
}

} // namespace UI
} // namespace Acheron

