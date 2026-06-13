#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QVector>

#include "Presence.hpp"
#include "Snowflake.hpp"

namespace Acheron {
namespace Core {

struct PlatformStatus {
    Platform platform;
    PresenceStatus status;
    bool operator==(const PlatformStatus &) const = default;
};

// Per-account store of the last-known presence status for users, fed by the
// member list and PRESENCE_UPDATE events. Any view that shows a user avatar can
// query statusOf() and repaint on presenceChanged().
class PresenceManager : public QObject
{
    Q_OBJECT
public:
    explicit PresenceManager(QObject *parent = nullptr) : QObject(parent) {}

    [[nodiscard]] PresenceStatus statusOf(Snowflake userId) const
    {
        return m_status.value(userId, PresenceStatus::Unknown);
    }

    void update(Snowflake userId, PresenceStatus status)
    {
        if (!userId.isValid() || status == PresenceStatus::Unknown)
            return;
        if (m_status.value(userId, PresenceStatus::Unknown) == status)
            return;
        m_status.insert(userId, status);
        emit presenceChanged(userId, status);
    }

    void update(Snowflake userId, const QString &status) { update(userId, presenceFromString(status)); }

    [[nodiscard]] const QVector<PlatformStatus> &platformsOf(Snowflake userId) const
    {
        auto it = m_platforms.constFind(userId);
        if (it != m_platforms.constEnd())
            return it.value();
        static const QVector<PlatformStatus> empty;
        return empty;
    }

    // platformStatuses: list of (platform-name, status-string) from a presence's
    // client_status object, e.g. {("desktop","online"),("mobile","idle")}.
    void updateClientStatus(Snowflake userId, const QList<QPair<QString, QString>> &platformStatuses)
    {
        if (!userId.isValid())
            return;

        QVector<PlatformStatus> next;
        next.reserve(platformStatuses.size());
        for (const auto &[name, statusStr] : platformStatuses) {
            const PresenceStatus s = presenceFromString(statusStr);
            if (s == PresenceStatus::Unknown)
                continue;
            next.append({ platformFromString(name), s });
        }

        auto existing = m_platforms.constFind(userId);
        if (existing != m_platforms.constEnd() && *existing == next)
            return;
        if (next.isEmpty())
            m_platforms.remove(userId);
        else
            m_platforms.insert(userId, std::move(next));
        emit presenceChanged(userId, statusOf(userId));
    }

signals:
    void presenceChanged(Acheron::Core::Snowflake userId, Acheron::Core::PresenceStatus status);

private:
    QHash<Snowflake, PresenceStatus> m_status;
    QHash<Snowflake, QVector<PlatformStatus>> m_platforms;
};

} // namespace Core
} // namespace Acheron
