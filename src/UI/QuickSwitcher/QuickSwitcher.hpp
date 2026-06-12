#pragma once

#include <QVector>
#include <QWidget>

#include "UI/ChannelList/ChannelTreeModel.hpp"

class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace Acheron {
namespace UI {

// Ctrl+K quick switcher: a small centered search popup that filters text
// channels and DMs across every connected account by name and opens the chosen
// one in a tab.
class QuickSwitcher : public QWidget
{
    Q_OBJECT
public:
    explicit QuickSwitcher(QWidget *parent = nullptr);

    void setResults(const QVector<ChannelTreeModel::ChannelSearchResult> &results);
    void showCentered(QWidget *over);

signals:
    void channelChosen(const ChannelTreeModel::ChannelSearchResult &result);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void refilter();
    void chooseCurrent();

    QLineEdit *m_search = nullptr;
    QListWidget *m_list = nullptr;
    QVector<ChannelTreeModel::ChannelSearchResult> m_all;
    QVector<ChannelTreeModel::ChannelSearchResult> m_filtered;
};

} // namespace UI
} // namespace Acheron
