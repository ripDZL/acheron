#include "MessageInput.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QAbstractTextDocumentLayout>
#include <QToolButton>
#include <QLabel>
#include <QSettings>
#include <atomic>

namespace Acheron {
namespace UI {

namespace {
// Live caches for the character-counter preferences (lazy-loaded from QSettings
// on first read, kept in sync by the setters). -1 = "not yet loaded".
std::atomic<int> g_counterEnabled{-1};
std::atomic<int> g_counterColorEffects{-1};
std::atomic<int> g_counterMax{-1};
} // namespace

bool MessageInput::counterEnabled()
{
    int v = g_counterEnabled.load(std::memory_order_relaxed);
    if (v < 0) {
        v = QSettings().value("input/char_counter", true).toBool() ? 1 : 0;
        g_counterEnabled.store(v, std::memory_order_relaxed);
    }
    return v != 0;
}

void MessageInput::setCounterEnabled(bool on)
{
    g_counterEnabled.store(on ? 1 : 0, std::memory_order_relaxed);
    QSettings().setValue("input/char_counter", on);
}

bool MessageInput::counterColorEffects()
{
    int v = g_counterColorEffects.load(std::memory_order_relaxed);
    if (v < 0) {
        v = QSettings().value("input/char_counter_color", true).toBool() ? 1 : 0;
        g_counterColorEffects.store(v, std::memory_order_relaxed);
    }
    return v != 0;
}

void MessageInput::setCounterColorEffects(bool on)
{
    g_counterColorEffects.store(on ? 1 : 0, std::memory_order_relaxed);
    QSettings().setValue("input/char_counter_color", on);
}

int MessageInput::counterMax()
{
    int v = g_counterMax.load(std::memory_order_relaxed);
    if (v < 0) {
        v = QSettings().value("input/char_counter_max", 2000).toInt();
        if (v < 1) v = 2000;
        g_counterMax.store(v, std::memory_order_relaxed);
    }
    return v;
}

void MessageInput::setCounterMax(int max)
{
    if (max < 1) max = 2000;
    g_counterMax.store(max, std::memory_order_relaxed);
    QSettings().setValue("input/char_counter_max", max);
}

ChatTextEdit::ChatTextEdit(QWidget *parent) : QTextEdit(parent)
{
    setObjectName("MessageInput");
    document()->setDocumentMargin(0);
    setAcceptRichText(false);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setPlaceholderText("Message...");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void ChatTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (!(e->modifiers() & Qt::ShiftModifier)) {
            emit returnPressed();
            return;
        }
    }
    if (e->key() == Qt::Key_Escape) {
        emit escapePressed();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

MessageInput::MessageInput(QWidget *parent) : QWidget(parent)
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(4, 0, 4, 0);
    outerLayout->setSpacing(0);

    // Reply bar
    replyBar = new QWidget(this);
    replyBar->setVisible(false);
    auto *replyLayout = new QHBoxLayout(replyBar);
    replyLayout->setContentsMargins(8, 4, 4, 2);
    replyLayout->setSpacing(4);

    replyLabel = new QLabel(replyBar);
    replyLabel->setStyleSheet("color: #b5bac1; font-size: 12px;");
    replyLayout->addWidget(replyLabel, 1);

    replyCancelButton = new QToolButton(replyBar);
    replyCancelButton->setText(QStringLiteral("\u00D7")); // multiplication sign as close icon
    replyCancelButton->setFixedSize(16, 16);
    replyCancelButton->setStyleSheet(
            "QToolButton { border: none; color: #b5bac1; font-size: 14px; }"
            "QToolButton:hover { color: #ffffff; }");
    replyLayout->addWidget(replyCancelButton);

    connect(replyCancelButton, &QToolButton::clicked, this, &MessageInput::clearReplyTarget);

    outerLayout->addWidget(replyBar);

    // Text edit
    auto *inputContainer = new QWidget(this);
    auto *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(0);

    textEdit = new ChatTextEdit(inputContainer);
    setFocusProxy(textEdit);

    // Character counter overlaid on the bottom-right of the text edit. It is
    // transparent to mouse events so it never blocks typing/selection.
    charCounter = new QLabel(textEdit);
    charCounter->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    charCounter->setTextInteractionFlags(Qt::NoTextInteraction);
    charCounter->setVisible(false);
    {
        QFont cf = charCounter->font();
        cf.setPixelSize(11);
        charCounter->setFont(cf);
    }

    connect(textEdit, &ChatTextEdit::returnPressed, [this]() {
        if (sendBlocked)
            return;
        QString txt = textEdit->toPlainText().trimmed();
        if (!txt.isEmpty()) {
            emit sendMessage(txt);
            clear();
        }
    });

    connect(textEdit, &ChatTextEdit::escapePressed, this, &MessageInput::clearReplyTarget);

    connect(textEdit->document(), &QTextDocument::contentsChanged, this,
            &MessageInput::adjustHeight);
    connect(textEdit->document(), &QTextDocument::contentsChanged, this,
            &MessageInput::updateCharCounter);
    connect(textEdit, &QTextEdit::selectionChanged, this,
            &MessageInput::updateCharCounter);

    inputLayout->addWidget(textEdit);
    outerLayout->addWidget(inputContainer);

    adjustHeight();
    updateCharCounter();
}

void MessageInput::updateCharCounter()
{
    if (!charCounter)
        return;

    if (!counterEnabled()) {
        charCounter->setVisible(false);
        return;
    }

    const QString text = textEdit->toPlainText();
    const int len = text.length();
    if (len == 0) {
        charCounter->setVisible(false);
        return;
    }

    const int max = counterMax();

    // Selected-text count (shown as "selected/" prefix, like Equicord).
    const int selected = textEdit->textCursor().selectedText().length();

    // Colour escalation matching Equicord's thresholds.
    QString color;
    if (!counterColorEffects()) {
        color = QStringLiteral("#949ba4"); // muted
    } else {
        const double pct = (static_cast<double>(len) / max) * 100.0;
        if (pct < 50.0)      color = QStringLiteral("#949ba4"); // muted
        else if (pct < 75.0) color = QStringLiteral("#e6c200"); // yellow
        else if (pct < 90.0) color = QStringLiteral("#e67e22"); // orange
        else                 color = QStringLiteral("#f23f43"); // red
    }

    QString html;
    if (selected > 0)
        html = QStringLiteral("<span style='color:#4285F4'>%1</span>/%2/%3")
                       .arg(selected).arg(len).arg(max);
    else
        html = QStringLiteral("%1/%2").arg(len).arg(max);

    charCounter->setText(html);
    charCounter->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg(color));
    charCounter->setVisible(true);
    charCounter->adjustSize();
    repositionCharCounter();
}

void MessageInput::repositionCharCounter()
{
    if (!charCounter || !charCounter->isVisible())
        return;
    // Bottom-right, with a small margin, inside the text edit's viewport.
    const int margin = 6;
    const int x = textEdit->width() - charCounter->width() - margin;
    const int y = textEdit->height() - charCounter->height() - margin;
    charCounter->move(qMax(margin, x), qMax(0, y));
}


void MessageInput::setPlaceholder(const QString &name)
{
    textEdit->setPlaceholderText(name);
}

void MessageInput::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    repositionCharCounter();
}

void MessageInput::clear()
{
    textEdit->clear();
    clearReplyTarget();
    adjustHeight();
}

void MessageInput::setReplyTarget(Core::Snowflake messageId, const QString &authorName,
                                  const QString &contentSnippet)
{
    replyMessageId = messageId;
    QString snippet = contentSnippet;
    snippet.replace('\n', ' ');
    if (snippet.length() > 100)
        snippet = snippet.left(100) + "...";

    replyLabel->setText(tr("Replying to <b>%1</b> %2").arg(authorName, snippet));
    replyBar->setVisible(true);
    adjustHeight();
    textEdit->setFocus();
}

void MessageInput::clearReplyTarget()
{
    if (!replyMessageId.isValid())
        return;

    replyMessageId = Core::Snowflake::Invalid;
    replyBar->setVisible(false);
    adjustHeight();
}

void MessageInput::setSendBlocked(bool blocked)
{
    sendBlocked = blocked;
}

void MessageInput::insertText(const QString &text)
{
    textEdit->insertPlainText(text);
    textEdit->setFocus();
}

void MessageInput::adjustHeight()
{
    int contentHeight = textEdit->document()->size().height();

    int newHeight = contentHeight + contentHeight;

    if (newHeight < 44)
        newHeight = 44;
    if (newHeight > 200)
        newHeight = 200;

    if (newHeight >= 200)
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    else
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    textEdit->setFixedHeight(newHeight);

    int totalHeight = newHeight + 12;
    if (replyBar->isVisible())
        totalHeight += replyBar->sizeHint().height();

    setFixedHeight(totalHeight);
    repositionCharCounter();
}

} // namespace UI
} // namespace Acheron