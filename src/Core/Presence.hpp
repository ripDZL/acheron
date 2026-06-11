#pragma once

#include <QColor>
#include <QString>

namespace Acheron {
namespace Core {

enum class PresenceStatus {
    Unknown,
    Offline,
    Online,
    Idle,
    Dnd,
};

inline PresenceStatus presenceFromString(const QString &s)
{
    if (s == QStringLiteral("online"))
        return PresenceStatus::Online;
    if (s == QStringLiteral("idle"))
        return PresenceStatus::Idle;
    if (s == QStringLiteral("dnd"))
        return PresenceStatus::Dnd;
    if (s == QStringLiteral("offline") || s == QStringLiteral("invisible"))
        return PresenceStatus::Offline;
    return PresenceStatus::Unknown;
}

// Discord-style status dot colors.
inline QColor presenceDotColor(PresenceStatus status)
{
    switch (status) {
    case PresenceStatus::Online:
        return QColor(0x23, 0xa5, 0x59); // green
    case PresenceStatus::Idle:
        return QColor(0xf0, 0xb2, 0x32); // amber
    case PresenceStatus::Dnd:
        return QColor(0xf2, 0x3f, 0x43); // red
    case PresenceStatus::Offline:
        return QColor(0x80, 0x84, 0x8e); // gray
    case PresenceStatus::Unknown:
    default:
        return QColor(); // invalid -> caller should not draw
    }
}

} // namespace Core
} // namespace Acheron
