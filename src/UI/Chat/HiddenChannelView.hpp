#pragma once

#include <QColor>
#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace Acheron {
namespace UI {

// Shown in place of the message list when a no-access ("hidden") channel is
// selected: explains the channel can't be read and lists who can access it.
class HiddenChannelView : public QWidget
{
    Q_OBJECT
public:
    struct AccessEntry {
        QString name;
        QColor color; // role colour, or invalid for members/default
    };

    explicit HiddenChannelView(QWidget *parent = nullptr);

    void setChannel(const QString &channelName, const QList<AccessEntry> &roles,
                    const QList<AccessEntry> &members, bool adminsAlways);

private:
    void clearList();

    QLabel *m_title = nullptr;
    QWidget *m_listContainer = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
};

} // namespace UI
} // namespace Acheron
