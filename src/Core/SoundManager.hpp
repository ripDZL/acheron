#pragma once

#include <QObject>
#include <QString>

namespace Acheron {
namespace Core {

// Plays short notification sounds (voice join/leave, connection lost).
// Backed by its own miniaudio engine instance, independent of the voice
// AudioPipeline so notification playback never interferes with a call.
//
// Sounds are bundled as Qt resources and extracted to a temp file on first
// use (miniaudio decodes from a filesystem path). Playback is fire-and-forget
// and non-blocking; overlapping sounds are mixed by the engine.
class SoundManager : public QObject
{
    Q_OBJECT

public:
    enum class Sound {
        UserConnect,
        UserDisconnect,
        ConnectionLost,
    };

    static SoundManager &instance();

    // Plays the given sound if notification sounds are enabled. Safe to call
    // from the GUI thread; returns immediately.
    void play(Sound sound);

    // Settings-backed master toggle (persisted under audio/notification_sounds).
    static bool soundsEnabled();
    static void setSoundsEnabled(bool enabled);

private:
    explicit SoundManager(QObject *parent = nullptr);
    ~SoundManager() override;

    SoundManager(const SoundManager &) = delete;
    SoundManager &operator=(const SoundManager &) = delete;

    bool ensureEngine();
    QString resourcePath(Sound sound) const;
    QString materialize(Sound sound);

    struct Impl;
    Impl *d = nullptr;
};

} // namespace Core
} // namespace Acheron
