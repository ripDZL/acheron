#include "UI/Search/SearchPanel.hpp"

#include "Core/Theme/Manager.hpp"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace Acheron {
namespace UI {

using Core::Theme::Token;
using Core::Theme::FontRole;

namespace {
constexpr int kDebounceMs = 250;

// Roles for stashing message location on each list item.
constexpr int ChannelIdRole = Qt::UserRole + 1;
constexpr int MessageIdRole = Qt::UserRole + 2;
} // namespace

SearchPanel::SearchPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    input = new QLineEdit(this);
    input->setPlaceholderText(tr("Search   e.g. from:alice has:image before:2025-01-01"));
    input->setClearButtonEnabled(true);
    layout->addWidget(input);

    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);

    resultList = new QListWidget(this);
    resultList->setUniformItemSizes(false);
    resultList->setWordWrap(true);
    resultList->setSelectionMode(QAbstractItemView::SingleSelection);
    resultList->setFrameShape(QFrame::NoFrame);
    layout->addWidget(resultList, 1);

    debounce = new QTimer(this);
    debounce->setSingleShot(true);
    debounce->setInterval(kDebounceMs);

    connect(input, &QLineEdit::textChanged, this, &SearchPanel::onTextChanged);
    connect(debounce, &QTimer::timeout, this, [this]() {
        const QString text = input->text().trimmed();
        if (text.isEmpty()) {
            setIdle();
            return;
        }
        setSearching();
        emit searchRequested(text);
    });
    connect(input, &QLineEdit::returnPressed, this, [this]() {
        debounce->stop();
        const QString text = input->text().trimmed();
        if (!text.isEmpty()) {
            setSearching();
            emit searchRequested(text);
        }
    });
    connect(resultList, &QListWidget::itemActivated, this, &SearchPanel::onItemActivated);
    connect(resultList, &QListWidget::itemClicked, this, &SearchPanel::onItemActivated);

    connect(&Core::Theme::Manager::instance(), &Core::Theme::Manager::themeChanged,
            this, &SearchPanel::applyTheme);

    applyTheme();
    setIdle();
}

void SearchPanel::reset()
{
    input->clear();
    resultList->clear();
    setIdle();
}

void SearchPanel::focusInput()
{
    input->setFocus();
    input->selectAll();
}

void SearchPanel::onTextChanged()
{
    debounce->start();
}

void SearchPanel::setSearching()
{
    statusLabel->setText(tr("Searching…"));
}

void SearchPanel::setIdle()
{
    resultList->clear();
    statusLabel->setText(tr("Search messages this account has synced locally."));
}

QString SearchPanel::relativeTimestamp(const QDateTime &ts) const
{
    if (!ts.isValid())
        return QString();
    return ts.toLocalTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
}

void SearchPanel::setResults(const QList<Discord::Message> &messages, int totalCount,
                             const QString &queryEcho)
{
    Q_UNUSED(queryEcho);
    resultList->clear();

    if (messages.isEmpty()) {
        statusLabel->setText(tr("No messages found."));
        return;
    }

    if (totalCount > messages.size()) {
        statusLabel->setText(tr("%1 results — showing first %2")
                                     .arg(totalCount)
                                     .arg(messages.size()));
    } else {
        statusLabel->setText(tr("%n result(s)", "", totalCount));
    }

    for (const Discord::Message &msg : messages) {
        QString author;
        if (msg.author.hasValue() && msg.author->globalName.hasValue()
            && !msg.author->globalName->isEmpty())
            author = msg.author->globalName.get();
        else if (msg.author.hasValue())
            author = msg.author->username.get();
        else
            author = tr("Unknown");

        const QString when = relativeTimestamp(msg.timestamp);
        QString preview = msg.content;
        preview.replace(QLatin1Char('\n'), QLatin1Char(' '));
        if (preview.length() > 280)
            preview = preview.left(280) + QStringLiteral("…");
        if (preview.isEmpty())
            preview = tr("(no text content)");

        auto *item = new QListWidgetItem(
            QStringLiteral("%1  ·  %2\n%3").arg(author, when, preview));
        item->setData(ChannelIdRole,
                      QVariant::fromValue<qulonglong>(static_cast<qulonglong>(msg.channelId.get())));
        item->setData(MessageIdRole,
                      QVariant::fromValue<qulonglong>(static_cast<qulonglong>(msg.id.get())));
        resultList->addItem(item);
    }
}

void SearchPanel::onItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const Core::Snowflake channelId(item->data(ChannelIdRole).toULongLong());
    const Core::Snowflake messageId(item->data(MessageIdRole).toULongLong());
    if (channelId.isValid())
        emit resultActivated(channelId, messageId);
}

void SearchPanel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit closeRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SearchPanel::applyTheme()
{
    auto &t = Core::Theme::Manager::instance();
    const QColor bg = t.color(Token::BaseBg);
    const QColor text = t.color(Token::PrimaryText);
    const QColor placeholder = t.color(Token::PlaceholderText);
    const QColor divider = t.color(Token::Divider);
    const QColor highlight = t.color(Token::Highlight);
    const QColor altBg = t.color(Token::AlternateBaseBg);

    setStyleSheet(QStringLiteral(
        "QWidget { background-color: %1; color: %2; }"
        "QLineEdit { background-color: %6; border: 1px solid %3; border-radius: 4px;"
        "            padding: 5px 8px; color: %2; }"
        "QLabel { color: %4; padding: 2px; }"
        "QListWidget { background-color: %1; border: none; }"
        "QListWidget::item { padding: 6px 4px; border-bottom: 1px solid %3; }"
        "QListWidget::item:selected { background-color: %5; color: %2; }")
        .arg(bg.name(), text.name(), divider.name(), placeholder.name(),
             highlight.name(), altBg.name()));

    statusLabel->setFont(t.font(FontRole::Ui));
    resultList->setFont(t.font(FontRole::Ui));
    input->setFont(t.font(FontRole::Ui));
}

} // namespace UI
} // namespace Acheron
