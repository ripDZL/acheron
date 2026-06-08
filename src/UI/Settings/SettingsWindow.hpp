#pragma once

#include <QtWidgets>

namespace Acheron {
namespace UI {

class SettingsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

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

    // discord
    QCheckBox *showNicknamesCheckbox = nullptr;
    QCheckBox *showTypingCheckbox = nullptr;
    QSpinBox *avatarSizeSpin = nullptr;
    QSpinBox *emojiSizeSpin = nullptr;

    // language
    QComboBox *languageCombo = nullptr;
};

} // namespace UI
} // namespace Acheron
