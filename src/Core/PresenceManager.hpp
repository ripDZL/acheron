#pragma once

#include <QHash>
#include <QObject>

#include "Presence.hpp"
#include "Snowflake.hpp"

namespace Acheron {
namespace Core {

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

signals:
    void presenceChanged(Acheron::Core::Snowflake userId, Acheron::Core::PresenceStatus status);

private:
    QHash<Snowflake, PresenceStatus> m_status;
};

} // namespace Core
} // namespace Acheron
