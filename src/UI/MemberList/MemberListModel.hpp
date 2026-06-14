#pragma once

#include <QAbstractListModel>

#include "Core/MemberListManager.hpp"
#include "Core/PresenceManager.hpp"
#include "Core/ImageManager.hpp"

namespace Acheron {
namespace UI {

class MemberListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ItemTypeRole = Qt::UserRole + 1,
        UserIdRole,
        UsernameRole,
        AvatarRole,
        RoleColorRole,
        GroupNameRole,
        GroupCountRole,
        GroupColorRole,
        LoadedRole,
        StatusRole,
        IsOwnerRole,
    };

    explicit MemberListModel(Core::ImageManager *imageManager, QObject *parent = nullptr);

    void setManager(Core::MemberListManager *manager);
    void setPresenceManager(Core::PresenceManager *presences);

    // Sets the current guild's owner so the view can mark them with a crown.
    // Pass an invalid id for non-guild contexts (e.g. DMs).
    void setGuildOwnerId(Core::Snowflake ownerId);

    // Live, QSettings-backed toggle for showing the owner crown.
    static bool showOwnerCrown();
    static void setShowOwnerCrown(bool on);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

private:
    Core::PresenceManager *presenceManager = nullptr;
    void onListAboutToReset();
    void onListReset();
    void onImageFetched(const QUrl &url, const QSize &size, const QPixmap &pixmap);

    void connectManager();
    void disconnectManager();

    Core::MemberListManager *manager = nullptr;
    Core::ImageManager *imageManager;
    Core::Snowflake guildOwnerId;

    mutable QHash<QUrl, QList<int>> pendingAvatars;
};

} // namespace UI
} // namespace Acheron
