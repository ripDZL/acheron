#pragma once

#include <QSqlDatabase>
#include "Core/SearchQueryParser.hpp"
#include <QSqlQuery>

#include "BaseRepository.hpp"
#include "Core/Snowflake.hpp"
#include "Discord/Entities.hpp"
#include "UserRepository.hpp"

namespace Acheron {
namespace Storage {

class MessageRepository : public QObject, public BaseRepository
{
    Q_OBJECT
public:
    MessageRepository(Core::Snowflake accountId);

    void saveMessages(const QList<Discord::Message> &messages);
    void saveMessages(const QList<Discord::Message> &messages, QSqlDatabase &db);
    void markMessageDeleted(Core::Snowflake messageId);
    void updateReactionsJson(Core::Snowflake messageId, const QString &reactionsJson);
    QString getReactionsJson(Core::Snowflake messageId);

    QList<Discord::Message> getLatestMessages(Core::Snowflake channelId, int limit);
    QList<Discord::Message> getMessagesBefore(Core::Snowflake channelId, Core::Snowflake beforeId,
                                              int limit);

    // Result of a search: a page of matching messages plus the total match
    // count (so the UI can show "X results" and paginate).
    struct SearchResult {
        QList<Discord::Message> messages;
        int totalCount = 0;
    };

    // Full-text-ish search over locally-cached messages. The query's name-based
    // operators (from/in/mentions) must already be resolved to ids by the
    // caller. offset/limit paginate the result, newest first.
    SearchResult searchMessages(const Core::SearchQuery &query, int offset, int limit);

private:
    void loadAttachmentsForMessages(QList<Discord::Message> &messages, QSqlDatabase &db);
    Discord::Message readMessageFromQuery(const QSqlQuery &q);

    UserRepository userRepository;
};

} // namespace Storage
} // namespace Acheron
