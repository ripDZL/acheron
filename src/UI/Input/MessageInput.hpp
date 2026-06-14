#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>

#include "Core/Snowflake.hpp"

class QToolButton;

namespace Acheron {
namespace UI {

class ChatTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ChatTextEdit(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;
signals:
    void returnPressed();
    void escapePressed();
};

class MessageInput : public QWidget
{
    Q_OBJECT
public:
    explicit MessageInput(QWidget *parent = nullptr);
    void clear();
    void setPlaceholder(const QString &name);

    void setReplyTarget(Core::Snowflake messageId, const QString &authorName,
                        const QString &contentSnippet);
    void clearReplyTarget();
    [[nodiscard]] Core::Snowflake replyTargetMessageId() const { return replyMessageId; }

    void setSendBlocked(bool blocked);
    [[nodiscard]] bool isSendBlocked() const { return sendBlocked; }

    void insertText(const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void sendMessage(const QString &text);

private:
    ChatTextEdit *textEdit;
    QWidget *replyBar;
    QLabel *replyLabel;
    QToolButton *replyCancelButton;
    QLabel *charCounter = nullptr;

    Core::Snowflake replyMessageId;
    bool sendBlocked = false;

    void adjustHeight();
    void updateCharCounter();
    void repositionCharCounter();

public:
    // Live, QSettings-backed character-counter preferences.
    // Master on/off:
    static bool counterEnabled();
    static void setCounterEnabled(bool on);
    // Colour escalation as the limit approaches:
    static bool counterColorEffects();
    static void setCounterColorEffects(bool on);
    // Character limit (defaults to 2000; settable so a Nitro-aware caller can
    // raise it to 4000 if the self user's premium type becomes known).
    static int counterMax();
    static void setCounterMax(int max);

private:
};

} // namespace UI
} // namespace Acheron