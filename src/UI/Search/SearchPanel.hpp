#pragma once

#include "Core/Snowflake.hpp"
#include "Discord/Entities.hpp"
#include "Storage/MessageRepository.hpp"

#include <QWidget>

class QLineEdit;
class QListWidget;
class QLabel;
class QListWidgetItem;

namespace Acheron {
namespace UI {

// A Discord-style message search panel that docks on the right of the chat.
// The panel takes a raw query string, asks the active account to run a local
// search, and lists the matching messages. Clicking a result asks the host to
// open that message's channel.
//
// The panel is intentionally presentation-only: it has no direct access to the
// account/repository. The host (MainWindow) connects searchRequested to perform
// the query and calls setResults with the outcome, and listens for
// resultActivated to navigate.
class SearchPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPanel(QWidget *parent = nullptr);

    // Clears the input and results (e.g. when switching accounts).
    void reset();

    // Puts keyboard focus in the search box.
    void focusInput();

    // Populates the result list. totalCount may exceed messages.size() when the
    // result set is paginated; the panel shows the count and a truncation hint.
    void setResults(const QList<Discord::Message> &messages, int totalCount,
                    const QString &queryEcho);

    // Shows a transient state while a search runs / before the first query.
    void setSearching();
    void setIdle();

signals:
    // Emitted (debounced) when the user has entered a query to run.
    void searchRequested(const QString &queryText);
    // Emitted when the user activates a result; carries the message location.
    void resultActivated(Core::Snowflake channelId, Core::Snowflake messageId);
    // Emitted when the user asks to close the panel (Esc or close button).
    void closeRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyTheme();
    void onTextChanged();
    void onItemActivated(QListWidgetItem *item);
    QString relativeTimestamp(const QDateTime &ts) const;

    QLineEdit *input = nullptr;
    QLabel *statusLabel = nullptr;
    QListWidget *resultList = nullptr;

    class QTimer *debounce = nullptr;
};

} // namespace UI
} // namespace Acheron
