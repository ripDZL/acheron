// SoundManager plays short notification sounds via miniaudio's high-level
// engine API. The miniaudio implementation is compiled once in
// MiniaudioImpl.cpp; this file (and MiniaudioAudioBackend.cpp) include
// MiniaudioConfig.hpp for declarations only, so all ma_* symbols have a single
// definition program-wide.

// Declarations only; the implementation lives in MiniaudioImpl.cpp.
#include "Core/AV/MiniaudioConfig.hpp"

#include "SoundManager.hpp"

#include "Core/Logging.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QHash>

namespace Acheron {
namespace Core {

namespace {
constexpr const char *kSettingsKey = "audio/notification_sounds";
}

struct SoundManager::Impl {
    ma_engine engine = {};
    bool engineReady = false;

    // Cache of extracted temp file paths, keyed by Sound enum value.
    QHash<int, QString> materialized;
};

SoundManager &SoundManager::instance()
{
    static SoundManager s;
    return s;
}

SoundManager::SoundManager(QObject *parent)
    : QObject(parent), d(new Impl)
{
}

SoundManager::~SoundManager()
{
    if (d->engineReady)
        ma_engine_uninit(&d->engine);
    delete d;
}

bool SoundManager::soundsEnabled()
{
    return QSettings().value(kSettingsKey, true).toBool();
}

void SoundManager::setSoundsEnabled(bool enabled)
{
    QSettings().setValue(kSettingsKey, enabled);
}

bool SoundManager::ensureEngine()
{
    if (d->engineReady)
        return true;

    ma_engine_config cfg = ma_engine_config_init();
    ma_result r = ma_engine_init(&cfg, &d->engine);
    if (r != MA_SUCCESS) {
        qCWarning(LogVoice) << "SoundManager: failed to init audio engine, code" << r;
        return false;
    }
    d->engineReady = true;
    return true;
}

QString SoundManager::resourcePath(Sound sound) const
{
    switch (sound) {
    case Sound::UserConnect:
        return QStringLiteral(":/sounds/User-Connect.wav");
    case Sound::UserDisconnect:
        return QStringLiteral(":/sounds/User-Disconnect.wav");
    case Sound::ConnectionLost:
        return QStringLiteral(":/sounds/connection-Lost.wav");
    }
    return QString();
}

QString SoundManager::materialize(Sound sound)
{
    const int key = static_cast<int>(sound);
    auto it = d->materialized.find(key);
    if (it != d->materialized.end() && QFileInfo::exists(it.value()))
        return it.value();

    const QString res = resourcePath(sound);
    QFile in(res);
    if (!in.open(QIODevice::ReadOnly)) {
        qCWarning(LogVoice) << "SoundManager: missing resource" << res;
        return QString();
    }

    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath();
    QDir().mkpath(dir);

    const QString outPath = dir + QDir::separator()
        + QStringLiteral("acheron-") + QFileInfo(res).fileName();
    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(LogVoice) << "SoundManager: cannot write temp sound" << outPath;
        return QString();
    }
    out.write(in.readAll());
    out.close();

    d->materialized.insert(key, outPath);
    return outPath;
}

void SoundManager::play(Sound sound)
{
    if (!soundsEnabled())
        return;
    if (!ensureEngine())
        return;

    const QString path = materialize(sound);
    if (path.isEmpty())
        return;

    // Fire-and-forget: miniaudio loads, plays, and frees the inline sound.
    const QByteArray utf8 = path.toUtf8();
    ma_result r = ma_engine_play_sound(&d->engine, utf8.constData(), nullptr);
    if (r != MA_SUCCESS)
        qCWarning(LogVoice) << "SoundManager: play failed for" << path << "code" << r;
}

} // namespace Core
} // namespace Acheron
