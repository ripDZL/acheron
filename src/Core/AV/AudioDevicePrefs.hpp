#pragma once

#include <QByteArray>
#include <QDebug>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QString>

namespace Acheron {
namespace Core {
namespace AV {

// Process-global, race-free store for the currently selected audio devices.
//
// The Settings > Audio tab, the Voice panel, and the voice connection all need
// to agree on the selected input/output device. Routing that hand-off through
// QSettings alone is unreliable within a single session: a fresh QSettings
// instance can serve a cached value, and sync() can miss a change made within
// the filesystem's mtime granularity. This holds the live values in memory
// (immediately visible to every reader) while still persisting to QSettings so
// the choice survives a restart.
class AudioDevicePrefs
{
public:
    static AudioDevicePrefs &instance()
    {
        static AudioDevicePrefs prefs;
        return prefs;
    }

    void setInput(const QByteArray &id, const QString &name)
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        m_inputId = id;
        m_inputName = name;
        qInfo().noquote() << "AudioDevicePrefs::setInput name=" << name << "idBytes=" << id.size();
        QSettings s;
        s.setValue(QStringLiteral("voice/input_device"), id);
        s.setValue(QStringLiteral("voice/input_device_name"), name);
    }

    void setOutput(const QByteArray &id, const QString &name)
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        m_outputId = id;
        m_outputName = name;
        qInfo().noquote() << "AudioDevicePrefs::setOutput name=" << name << "idBytes=" << id.size();
        QSettings s;
        s.setValue(QStringLiteral("voice/output_device"), id);
        s.setValue(QStringLiteral("voice/output_device_name"), name);
    }

    QByteArray inputId()
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        return m_inputId;
    }
    QString inputName()
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        return m_inputName;
    }
    QByteArray outputId()
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        return m_outputId;
    }
    QString outputName()
    {
        QMutexLocker lock(&m_mutex);
        ensureLoaded();
        return m_outputName;
    }

private:
    AudioDevicePrefs() = default;

    // Loads persisted values once (subsequent setters keep the in-memory copy
    // authoritative, so we never re-read a potentially stale QSettings value).
    void ensureLoaded()
    {
        if (m_loaded)
            return;
        QSettings s;
        m_inputId = s.value(QStringLiteral("voice/input_device")).toByteArray();
        m_inputName = s.value(QStringLiteral("voice/input_device_name")).toString();
        m_outputId = s.value(QStringLiteral("voice/output_device")).toByteArray();
        m_outputName = s.value(QStringLiteral("voice/output_device_name")).toString();
        m_loaded = true;
    }

    QMutex m_mutex;
    bool m_loaded = false;
    QByteArray m_inputId;
    QByteArray m_outputId;
    QString m_inputName;
    QString m_outputName;
};

} // namespace AV
} // namespace Core
} // namespace Acheron
