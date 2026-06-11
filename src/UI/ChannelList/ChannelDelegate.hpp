#pragma once

#include <QtWidgets>

class QAbstractProxyModel;

namespace Acheron {
namespace Core {
class PresenceManager;
}
namespace UI {
class ChannelDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChannelDelegate(QAbstractProxyModel *proxyModel = nullptr, QObject *parent = nullptr);

    void setPresenceManager(Core::PresenceManager *presences) { presenceManager = presences; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QAbstractProxyModel *proxyModel;
    Core::PresenceManager *presenceManager = nullptr;
};
} // namespace UI
} // namespace Acheron
