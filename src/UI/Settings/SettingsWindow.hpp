#pragma once

#include <QtWidgets>

namespace Acheron {
namespace Core { namespace AV { class IAudioBackend; } }
namespace UI {

class SettingsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

signals:
    void showHiddenChannelsChanged();

private:
    void setupUi();
    void loadSettings();
    void buildGeneralPage();
    void buildDiscordPage();
    void buildAppearancePage();
    void buildLanguagePage();
    void buildAudioPage();

    QListWidget *categoryList;
    QStackedWidget *pages;

    // general
    QCheckBox *inMemoryCacheCheckbox = nullptr;
    QCheckBox *downloadImagesCheckbox = nullptr;
    QSpinBox *messageSpacingSpin = nullptr;
    QCheckBox *invertWheelCheckbox = nullptr;
    QDoubleSpinBox *scrollSpeedSpin = nullptr;
    QCheckBox *showCloseButtonCheckbox = nullptr;
    QCheckBox *extraActiveHighlightCheckbox = nullptr;
    QCheckBox *avoidRedundantTabsCheckbox = nullptr;
    QCheckBox *restorePreviousTabsCheckbox = nullptr;
    QCheckBox *showTrayIconCheckbox = nullptr;
    QCheckBox *closeToTrayCheckbox = nullptr;
    QCheckBox *showAccountsOnLaunchCheckbox = nullptr;

    // discord
    QCheckBox *showNicknamesCheckbox = nullptr;
    QCheckBox *notificationSoundsCheckbox = nullptr;
    QCheckBox *showTypingCheckbox = nullptr;
    QCheckBox *showHiddenChannelsCheckbox = nullptr;
    QSpinBox *avatarSizeSpin = nullptr;
    QSpinBox *emojiSizeSpin = nullptr;

    // language
    QComboBox *languageCombo = nullptr;

    // audio
    QComboBox *audioInputCombo = nullptr;
    QComboBox *audioOutputCombo = nullptr;
    QComboBox *inputChannelsCombo = nullptr;
    QComboBox *micModeCombo = nullptr;
    QKeySequenceEdit *pttHotkeyEdit = nullptr;
    QPushButton *pttHotkeyClear = nullptr;
    // Long-lived backend used only to enumerate audio devices. Kept alive for the
    // window's lifetime so we don't repeatedly create/destroy an audio context
    // (which can disturb Windows audio state for a subsequent voice connection).
    Core::AV::IAudioBackend *enumBackend = nullptr;
    void populateAudioDevices();
};

} // namespace UI
} // namespace Acheron
