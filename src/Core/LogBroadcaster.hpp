#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include <deque>
#include <mutex>

namespace Acheron {
namespace Core {

// Receives every formatted log line from the Qt message handler and makes it
// available to the in-app event log viewer: it keeps a thread-safe ring buffer
// of recent lines (so a newly-opened viewer can show history) and emits a live
// signal for each new line. publish() is safe to call from any thread; the
// signal is delivered to the viewer's thread via a queued connection.
class LogBroadcaster : public QObject
{
    Q_OBJECT
public:
    struct Entry {
        QString text;
        int type; // QtMsgType
    };

    static LogBroadcaster &instance();

    void publish(const QString &line, int type)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_recent.push_back({ line, type });
            while (m_recent.size() > kMaxRecent)
                m_recent.pop_front();
        }
        emit appended(line, type);
    }

    QVector<Entry> snapshot()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        QVector<Entry> out;
        out.reserve(static_cast<int>(m_recent.size()));
        for (const auto &e : m_recent)
            out.append(e);
        return out;
    }

signals:
    void appended(const QString &line, int type);

private:
    LogBroadcaster() = default;

    std::mutex m_mutex;
    std::deque<Entry> m_recent;
    static constexpr size_t kMaxRecent = 5000;
};

} // namespace Core
} // namespace Acheron
