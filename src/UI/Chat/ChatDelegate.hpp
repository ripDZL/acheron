#pragma once

#include <QStyledItemDelegate>

namespace Acheron {
namespace Core {
class ImageManager;
class PresenceManager;
}
namespace UI {
class ChatDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatDelegate(Core::ImageManager *imageManager, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), imageManager(imageManager)
    {
    }

    void setPresenceManager(Core::PresenceManager *presences) { presenceManager = presences; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Core::ImageManager *imageManager;
    Core::PresenceManager *presenceManager = nullptr;
};
} // namespace UI
} // namespace Acheron
