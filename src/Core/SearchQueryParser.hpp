#pragma once

#include "Core/Snowflake.hpp"

#include <QDateTime>
#include <QList>
#include <QString>

namespace Acheron {
namespace Core {

// Parsed representation of a Discord-style search query.
//
// Supported operators (mirroring Discord):
//   from:<user>      author filter   (resolved name -> id by the caller)
//   in:<channel>     channel filter  (resolved name -> id by the caller)
//   mentions:<user>  message mentions a user
//   has:link|image|video|file|embed|sound   attachment/embed predicates
//   before:<date>    messages before a date (YYYY-MM-DD)
//   after:<date>     messages after a date  (YYYY-MM-DD)
//   during:<date>    messages on a date     (YYYY-MM-DD)
// Everything not matching an operator becomes free-text search terms.
//
// Name-based operators (from/in/mentions) are captured as raw strings here;
// resolution to Snowflakes happens in a separate pass so the parser stays free
// of any dependency on the user/channel managers.
struct SearchQuery {
    enum class Has {
        Link,
        Image,
        Video,
        File,
        Embed,
        Sound,
    };

    // Free-text terms (AND-combined, case-insensitive substring match).
    QStringList terms;

    // Raw operator arguments, pre-resolution.
    QStringList fromNames;     // from:
    QStringList inNames;       // in:
    QStringList mentionsNames; // mentions:
    QList<Has> has;

    // Resolved ids (filled in by the resolution pass; empty until then).
    QList<Snowflake> fromIds;
    QList<Snowflake> inIds;
    QList<Snowflake> mentionsIds;

    // Date bounds (invalid QDateTime means unbounded).
    QDateTime before;
    QDateTime after;

    bool isEmpty() const
    {
        return terms.isEmpty() && fromNames.isEmpty() && inNames.isEmpty()
            && mentionsNames.isEmpty() && has.isEmpty() && fromIds.isEmpty()
            && inIds.isEmpty() && mentionsIds.isEmpty() && !before.isValid()
            && !after.isValid();
    }
};

class SearchQueryParser {
public:
    // Parses raw user input into a SearchQuery. Tokens of the form key:value
    // (optionally quoted: from:"Some Name") become operators; the rest are
    // free-text terms. Unknown keys are treated as plain text.
    static SearchQuery parse(const QString &input);

    // Renders a single Has value back to its keyword (for UI chips / tests).
    static QString hasKeyword(SearchQuery::Has has);
};

} // namespace Core
} // namespace Acheron
