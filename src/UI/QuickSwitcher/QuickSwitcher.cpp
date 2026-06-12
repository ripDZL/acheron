#include "QuickSwitcher.hpp"

#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace Acheron {
namespace UI {

QuickSwitcher::QuickSwitcher(QWidget *parent) : QWidget(parent, Qt::Popup)
{
    setObjectName("quickSwitcher");
    setFixedWidth(560);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search channels and direct messages…"));
    m_search->setClearButtonEnabled(true);
    layout->addWidget(m_search);

    m_list = new QListWidget(this);
    m_list->setUniformItemSizes(true);
    m_list->setFocusPolicy(Qt::NoFocus); // keep typing focus on the line edit
    layout->addWidget(m_list);

    m_search->installEventFilter(this);

    connect(m_search, &QLineEdit::textChanged, this, [this]() { refilter(); });
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *) { chooseCurrent(); });
}

void QuickSwitcher::setResults(const QVector<ChannelTreeModel::ChannelSearchResult> &results)
{
    m_all = results;
}

void QuickSwitcher::showCentered(QWidget *over)
{
    m_search->clear();
    refilter();
    adjustSize();
    resize(width(), 420);

    if (over) {
        QRect g = over->geometry();
        QPoint topLeft = over->mapToGlobal(QPoint(0, 0));
        int x = topLeft.x() + (g.width() - width()) / 2;
        int y = topLeft.y() + g.height() / 6;
        move(qMax(0, x), qMax(0, y));
    }

    show();
    raise();
    activateWindow();
    m_search->setFocus();
}

void QuickSwitcher::refilter()
{
    const QString q = m_search->text().trimmed();
    m_filtered.clear();

    if (q.isEmpty()) {
        m_filtered = m_all.mid(0, 50);
    } else {
        QVector<ChannelTreeModel::ChannelSearchResult> prefix, contains;
        for (const auto &r : m_all) {
            const int idx = r.name.indexOf(q, 0, Qt::CaseInsensitive);
            if (idx == 0)
                prefix.append(r);
            else if (idx > 0 || r.context.contains(q, Qt::CaseInsensitive))
                contains.append(r);
        }
        m_filtered = prefix;
        m_filtered += contains;
        if (m_filtered.size() > 50)
            m_filtered.resize(50);
    }

    m_list->clear();
    for (const auto &r : m_filtered) {
        const QString prefixChar = r.isDm ? QStringLiteral("@") : QStringLiteral("#");
        QString label = prefixChar + r.name;
        if (!r.context.isEmpty())
            label += QStringLiteral("   —   ") + r.context;
        m_list->addItem(label);
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void QuickSwitcher::chooseCurrent()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_filtered.size())
        return;
    const auto result = m_filtered.at(row);
    close();
    emit channelChosen(result);
}

bool QuickSwitcher::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_search && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Down:
            if (m_list->count() > 0)
                m_list->setCurrentRow(qMin(m_list->currentRow() + 1, m_list->count() - 1));
            return true;
        case Qt::Key_Up:
            if (m_list->count() > 0)
                m_list->setCurrentRow(qMax(m_list->currentRow() - 1, 0));
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            chooseCurrent();
            return true;
        case Qt::Key_Escape:
            close();
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void QuickSwitcher::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

void QuickSwitcher::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange && !isActiveWindow())
        close();
    QWidget::changeEvent(event);
}

} // namespace UI
} // namespace Acheron
