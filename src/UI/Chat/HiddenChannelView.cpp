#include "HiddenChannelView.hpp"

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace Acheron {
namespace UI {

HiddenChannelView::HiddenChannelView(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto *content = new QWidget(scroll);
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(40, 48, 40, 48);
    col->setSpacing(10);
    col->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    auto *lock = new QLabel(QStringLiteral("🔒"), content);
    QFont lockFont = lock->font();
    lockFont.setPointSize(34);
    lock->setFont(lockFont);
    lock->setAlignment(Qt::AlignHCenter);
    col->addWidget(lock);

    m_title = new QLabel(content);
    QFont titleFont = m_title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignHCenter);
    m_title->setWordWrap(true);
    col->addWidget(m_title);

    auto *subtitle = new QLabel(tr("You do not have access to this channel."), content);
    subtitle->setAlignment(Qt::AlignHCenter);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet("color: palette(mid);");
    col->addWidget(subtitle);

    col->addSpacing(8);

    m_listContainer = new QWidget(content);
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(4);
    m_listLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    col->addWidget(m_listContainer);

    col->addStretch(1);

    scroll->setWidget(content);
}

void HiddenChannelView::clearList()
{
    QLayoutItem *item = nullptr;
    while ((item = m_listLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
}

void HiddenChannelView::setChannel(const QString &channelName, const QList<AccessEntry> &roles,
                                   const QList<AccessEntry> &members, bool adminsAlways)
{
    m_title->setText(QStringLiteral("# %1").arg(channelName));
    clearList();

    auto addHeader = [this](const QString &text) {
        auto *h = new QLabel(text, m_listContainer);
        QFont f = h->font();
        f.setBold(true);
        h->setFont(f);
        h->setAlignment(Qt::AlignHCenter);
        h->setContentsMargins(0, 10, 0, 2);
        m_listLayout->addWidget(h);
    };
    auto addEntry = [this](const AccessEntry &e, const QString &prefix) {
        auto *label = new QLabel(prefix + e.name, m_listContainer);
        label->setAlignment(Qt::AlignHCenter);
        if (e.color.isValid())
            label->setStyleSheet(QStringLiteral("color: %1;").arg(e.color.name()));
        m_listLayout->addWidget(label);
    };

    if (roles.isEmpty() && members.isEmpty() && !adminsAlways) {
        auto *none = new QLabel(tr("No roles or members have explicit access."), m_listContainer);
        none->setAlignment(Qt::AlignHCenter);
        none->setStyleSheet("color: palette(mid);");
        m_listLayout->addWidget(none);
        return;
    }

    if (!roles.isEmpty()) {
        addHeader(tr("Roles with access"));
        for (const auto &r : roles)
            addEntry(r, QStringLiteral("@"));
    }
    if (!members.isEmpty()) {
        addHeader(tr("Members with access"));
        for (const auto &m : members)
            addEntry(m, QStringLiteral("@"));
    }
    if (adminsAlways) {
        auto *note = new QLabel(tr("Administrators can always access this channel."),
                                m_listContainer);
        note->setAlignment(Qt::AlignHCenter);
        note->setWordWrap(true);
        note->setStyleSheet("color: palette(mid);");
        note->setContentsMargins(0, 12, 0, 0);
        m_listLayout->addWidget(note);
    }
}

} // namespace UI
} // namespace Acheron
