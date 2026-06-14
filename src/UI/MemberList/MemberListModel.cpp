#include "MemberListModel.hpp"

#include "Discord/CdnUrls.hpp"

#include <QSettings>
#include <atomic>

namespace Acheron {
namespace UI {

namespace {
std::atomic<int> g_showOwnerCrown{-1}; // -1 = not yet loaded
}

bool MemberListModel::showOwnerCrown()
{
    int v = g_showOwnerCrown.load(std::memory_order_relaxed);
    if (v < 0) {
        v = QSettings().value("members/owner_crown", true).toBool() ? 1 : 0;
        g_showOwnerCrown.store(v, std::memory_order_relaxed);
    }
    return v != 0;
}

void MemberListModel::setShowOwnerCrown(bool on)
{
    g_showOwnerCrown.store(on ? 1 : 0, std::memory_order_relaxed);
    QSettings().setValue("members/owner_crown", on);
}

void MemberListModel::setGuildOwnerId(Core::Snowflake ownerId)
{
    if (guildOwnerId == ownerId)
        return;
    guildOwnerId = ownerId;
    // Repaint all rows so crowns update for the new guild context.
    if (manager && manager->totalItemCount() > 0)
        emit dataChanged(index(0, 0), index(manager->totalItemCount() - 1, 0), {IsOwnerRole});
}

constexpr static QSize AvatarRequestSize = QSize(32, 32);

MemberListModel::MemberListModel(Core::ImageManager *imageManager, QObject *parent)
    : QAbstractListModel(parent), imageManager(imageManager)
{
    connect(imageManager, &Core::ImageManager::imageFetched, this, &MemberListModel::onImageFetched);
}

void MemberListModel::setManager(Core::MemberListManager *newManager)
{
    beginResetModel();
    disconnectManager();
    manager = newManager;
    pendingAvatars.clear();
    connectManager();
    endResetModel();
}

void MemberListModel::setPresenceManager(Core::PresenceManager *presences)
{
    presenceManager = presences;
}

int MemberListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !manager)
        return 0;

    return manager->totalItemCount();
}

QVariant MemberListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !manager)
        return {};

    int row = index.row();
    if (row < 0 || row >= manager->totalItemCount())
        return {};

    const auto *item = manager->itemAt(row);

    if (!item) {
        switch (role) {
        case ItemTypeRole:
            return static_cast<int>(Core::MemberListItem::Type::Placeholder);
        case LoadedRole:
            return false;
        default:
            return {};
        }
    }

    switch (role) {
    case ItemTypeRole:
        return static_cast<int>(item->type);
    case LoadedRole:
        return true;
    case StatusRole:
        return item->type == Core::MemberListItem::Type::Member && presenceManager
                       ? static_cast<int>(presenceManager->statusOf(item->userId))
                       : static_cast<int>(Core::PresenceStatus::Unknown);
    case UserIdRole:
        return item->type == Core::MemberListItem::Type::Member
                       ? QVariant::fromValue(static_cast<quint64>(item->userId))
                       : QVariant();
    case IsOwnerRole:
        return item->type == Core::MemberListItem::Type::Member
                && guildOwnerId.isValid() && item->userId == guildOwnerId;
    case UsernameRole:
        return item->type == Core::MemberListItem::Type::Member
                       ? item->displayName
                       : QString();
    case AvatarRole: {
        if (item->type != Core::MemberListItem::Type::Member)
            return QVariant();

        if (!item->member.user->avatar.hasValue())
            return QVariant();

        Core::Snowflake userId = item->userId;
        QString avatarHash = item->member.user->avatar.get();
        if (avatarHash.isEmpty())
            return QVariant();

        QUrl url = Discord::Cdn::userAvatar(userId, avatarHash, AvatarRequestSize.width());

        if (imageManager->isCached(url, AvatarRequestSize))
            return imageManager->get(url, AvatarRequestSize);

        imageManager->get(url, AvatarRequestSize);
        if (!pendingAvatars.contains(url))
            pendingAvatars[url] = {};
        if (!pendingAvatars[url].contains(row))
            pendingAvatars[url].append(row);

        return QVariant();
    }
    case RoleColorRole:
        return item->type == Core::MemberListItem::Type::Member
                       ? QVariant::fromValue(item->roleColor)
                       : QVariant();
    case GroupNameRole:
        return item->type == Core::MemberListItem::Type::Group
                       ? item->groupName
                       : QString();
    case GroupCountRole:
        return item->type == Core::MemberListItem::Type::Group
                       ? item->groupCount
                       : 0;
    case GroupColorRole:
        return item->type == Core::MemberListItem::Type::Group
                       ? QVariant::fromValue(item->groupColor)
                       : QVariant();
    }

    return {};
}

void MemberListModel::onListAboutToReset()
{
    beginResetModel();
}

void MemberListModel::onListReset()
{
    pendingAvatars.clear();
    endResetModel();
}

void MemberListModel::onImageFetched(const QUrl &url, const QSize &size, const QPixmap &pixmap)
{
    Q_UNUSED(size);
    Q_UNUSED(pixmap);

    auto it = pendingAvatars.find(url);
    if (it == pendingAvatars.end())
        return;

    const QList<int> rows = it.value();
    pendingAvatars.erase(it);

    for (int row : rows) {
        if (row < rowCount()) {
            QModelIndex idx = createIndex(row, 0);
            emit dataChanged(idx, idx);
        }
    }
}

void MemberListModel::connectManager()
{
    if (!manager)
        return;

    connect(manager, &Core::MemberListManager::listAboutToReset,
            this, &MemberListModel::onListAboutToReset);
    connect(manager, &Core::MemberListManager::listReset,
            this, &MemberListModel::onListReset);
}

void MemberListModel::disconnectManager()
{
    if (!manager)
        return;

    disconnect(manager, nullptr, this, nullptr);
}

} // namespace UI
} // namespace Acheron
