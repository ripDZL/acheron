#pragma once

#include <QtWidgets>

#include "UI/Theme.hpp"

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
    QPushButton *makeColorSwatch(const QColor &initial);
    void setSwatchColor(QPushButton *btn, const QColor &c);
    void applyAndSaveTheme();

    QListWidget *categoryList;
    QStackedWidget *pages;

    // general
    QCheckBox *inMemoryCacheCheckbox = nullptr;
    QCheckBox *downloadImagesCheckbox = nullptr;
    QSpinBox *messageSpacingSpin = nullptr;

    // discord
    QCheckBox *showNicknamesCheckbox = nullptr;
    QCheckBox *showTypingCheckbox = nullptr;
    QSpinBox *avatarSizeSpin = nullptr;
    QSpinBox *emojiSizeSpin = nullptr;

    // language
    QComboBox *languageCombo = nullptr;

    // appearance
    ThemeColors currentTheme;
    QPushButton *bgSwatch = nullptr;
    QPushButton *textSwatch = nullptr;
    QPushButton *baseSwatch = nullptr;
    QPushButton *accentSwatch = nullptr;
    QPushButton *buttonSwatch = nullptr;
    QPushButton *borderSwatch = nullptr;
    QComboBox *fontCombo = nullptr;
    QSpinBox *fontSizeSpin = nullptr;
    QCheckBox *useCustomFont = nullptr;
};

} // namespace UI
} // namespace Acheron

