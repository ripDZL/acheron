#include "App.hpp"
#include "UI/MainWindow.hpp"
#include "UI/Theme.hpp"
#include "Storage/DatabaseManager.hpp"
#include "Core/Session.hpp"
#include "Core/Logging.hpp"
#include "Discord/CurlUtils.hpp"

#include <curl/curl.h>

#include <QtGlobal>
#include <QNetworkAccessManager>
#include <QFontDatabase>

#ifndef ACHERON_NO_VOICE
#  include <dave/dave.h>
#  include <dave/logger.h>
#endif

// potentially named after that river
// or the honkai star rail character
// who knows

void registerMetatypes()
{
    qRegisterMetaType<Acheron::Core::Snowflake>("Snowflake");

    QMetaType::registerConverter<Snowflake, QString>(
            [](const Snowflake &s) { return s.toString(); });
}

#ifndef ACHERON_NO_VOICE
static void DaveLogSink(discord::dave::LoggingSeverity severity, const char *file, int line, const std::string &message)
{
    switch (severity) {
    case discord::dave::LoggingSeverity::LS_ERROR:
        qCCritical(LogDave) << file << ":" << line << ": " << message.c_str();
        break;
    case discord::dave::LoggingSeverity::LS_WARNING:
        qCWarning(LogDave) << file << ":" << line << ": " << message.c_str();
        break;
    case discord::dave::LoggingSeverity::LS_INFO:
        qCInfo(LogDave) << file << ":" << line << ": " << message.c_str();
        break;
    case discord::dave::LoggingSeverity::LS_VERBOSE:
        qCDebug(LogDave) << file << ":" << line << ": " << message.c_str();
        break;
    case discord::dave::LoggingSeverity::LS_NONE:
        break;
    }
}
#endif

int main(int argc, char *argv[])
{
    using namespace Acheron;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    App app(argc, argv);
    app.setOrganizationName("ouwou");
    app.setApplicationName("Acheron");
    app.setStyle("Fusion");

    registerMetatypes();

    QNetworkAccessManager buildNumberNam;
    Discord::CurlUtils::fetchBuildNumber(&buildNumberNam);

#if 1
    {
        Acheron::UI::ThemeColors themeColors = Acheron::UI::Theme::load();
        Acheron::UI::Theme::apply(themeColors);
    }
#endif

#if 0
    QPalette warmPastelPalette;

    warmPastelPalette.setColor(QPalette::Window, QColor(255, 244, 230)); // soft cream background
    warmPastelPalette.setColor(QPalette::WindowText, QColor(85, 52, 52)); // muted brown text
    warmPastelPalette.setColor(QPalette::Base, QColor(255, 250, 240)); // input backgrounds
    warmPastelPalette.setColor(QPalette::AlternateBase,
                               QColor(255, 239, 220)); // slightly darker for alternating rows
    warmPastelPalette.setColor(QPalette::ToolTipBase, QColor(255, 250, 240)); // tooltip background
    warmPastelPalette.setColor(QPalette::ToolTipText, QColor(85, 52, 52)); // tooltip text
    warmPastelPalette.setColor(QPalette::Text, QColor(102, 68, 68)); // standard text
    warmPastelPalette.setColor(QPalette::Button, QColor(255, 214, 179)); // button background
    warmPastelPalette.setColor(QPalette::ButtonText, QColor(102, 68, 68)); // button text
    warmPastelPalette.setColor(QPalette::BrightText, QColor(255, 102, 102)); // for alerts
    warmPastelPalette.setColor(QPalette::Highlight, QColor(255, 179, 128)); // selection color
    warmPastelPalette.setColor(QPalette::HighlightedText, QColor(255, 244, 230)); // selection text

    qApp->setPalette(warmPastelPalette);
#endif

    Acheron::Core::Logger::init();

#ifndef ACHERON_NO_VOICE
    discord::dave::SetLogSink(DaveLogSink);
#endif

    qCInfo(LogCore) << "Starting Acheron...";

#ifdef Q_OS_WINDOWS
    int emojiFontId = QFontDatabase::addApplicationFont(
            QCoreApplication::applicationDirPath() + "/fonts/TwemojiCOLRv0.ttf");
    if (emojiFontId != -1) {
        QStringList families = QFontDatabase::applicationFontFamilies(emojiFontId);
        qCInfo(LogCore) << "Loaded emoji font:" << families;
        if (!families.isEmpty())
            QFontDatabase::addApplicationEmojiFontFamily(families.first());
    } else {
        qCWarning(LogCore) << "Failed to load TwemojiCOLRv0.ttf";
    }
#endif

    if (!Storage::DatabaseManager::instance().init()) {
        QMessageBox::critical(nullptr, "Fatal error",
                              "Could not initialize the database. Acheron will now close.");
        return -1;
    }

    int exitCode = 0;

    {
        Core::Session session;
        UI::MainWindow window(&session);
        window.show();

        exitCode = app.exec();
    }

    Storage::DatabaseManager::instance().shutdown();

    curl_global_cleanup();

    return exitCode;
}
