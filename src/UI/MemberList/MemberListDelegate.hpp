#pragma once

#include <QStyledItemDelegate>

namespace Acheron {
namespace Core {
class PresenceManager;
}
namespace UI {

class MemberListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MemberListDelegate(QObject *parent = nullptr);

    void setPresenceManager(Core::PresenceManager *presences) { presenceManager = presences; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    void paintGroup(QPainter *painter, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void paintMember(QPainter *painter, const QStyleOptionViewItem &option,
                     const QModelIndex &index) const;
    void paintPlaceholder(QPainter *painter, const QStyleOptionViewItem &option) const;

    Core::PresenceManager *presenceManager = nullptr;
};

} // namespace UI
} // namespace Acheron
