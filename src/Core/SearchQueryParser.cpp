#include "Core/SearchQueryParser.hpp"

#include <QRegularExpression>

namespace Acheron {
namespace Core {

namespace {

// Splits input into tokens, honoring double quotes so that
//   from:"Jane Doe" hello world
// yields tokens: [from:Jane Doe], [hello], [world].
QStringList tokenize(const QString &input)
{
    QStringList tokens;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < input.size(); ++i) {
        const QChar c = input.at(i);
        if (c == QLatin1Char('"')) {
            inQuotes = !inQuotes;
            continue;
        }
        if (c.isSpace() && !inQuotes) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
            continue;
        }
        current.append(c);
    }
    if (!current.isEmpty())
        tokens.append(current);
    return tokens;
}

// Parses a date operator value (YYYY-MM-DD) into a QDateTime at local midnight.
QDateTime parseDate(const QString &value)
{
    const QDate d = QDate::fromString(value, QStringLiteral("yyyy-MM-dd"));
    if (!d.isValid())
        return QDateTime();
    return QDateTime(d, QTime(0, 0), Qt::UTC);
}

bool parseHas(const QString &value, SearchQuery::Has &out)
{
    const QString v = value.toLower();
    if (v == QLatin1String("link")) { out = SearchQuery::Has::Link; return true; }
    if (v == QLatin1String("image")) { out = SearchQuery::Has::Image; return true; }
    if (v == QLatin1String("video")) { out = SearchQuery::Has::Video; return true; }
    if (v == QLatin1String("file")) { out = SearchQuery::Has::File; return true; }
    if (v == QLatin1String("embed")) { out = SearchQuery::Has::Embed; return true; }
    if (v == QLatin1String("sound")) { out = SearchQuery::Has::Sound; return true; }
    return false;
}

} // namespace

SearchQuery SearchQueryParser::parse(const QString &input)
{
    SearchQuery query;

    const QStringList tokens = tokenize(input.trimmed());
    for (const QString &token : tokens) {
        const int colon = token.indexOf(QLatin1Char(':'));
        // No colon, or colon at edges -> plain text term.
        if (colon <= 0 || colon == token.size() - 1) {
            if (!token.isEmpty())
                query.terms.append(token);
            continue;
        }

        const QString key = token.left(colon).toLower();
        const QString value = token.mid(colon + 1);

        if (key == QLatin1String("from")) {
            query.fromNames.append(value);
        } else if (key == QLatin1String("in")) {
            query.inNames.append(value);
        } else if (key == QLatin1String("mentions")) {
            query.mentionsNames.append(value);
        } else if (key == QLatin1String("has")) {
            SearchQuery::Has h;
            if (parseHas(value, h))
                query.has.append(h);
            // Unknown has: value is ignored (not treated as text).
        } else if (key == QLatin1String("before")) {
            const QDateTime dt = parseDate(value);
            if (dt.isValid())
                query.before = dt;
        } else if (key == QLatin1String("after")) {
            const QDateTime dt = parseDate(value);
            if (dt.isValid())
                query.after = dt;
        } else if (key == QLatin1String("during")) {
            const QDateTime dt = parseDate(value);
            if (dt.isValid()) {
                query.after = dt;
                query.before = dt.addDays(1);
            }
        } else {
            // Unknown operator -> treat whole token as a text term.
            query.terms.append(token);
        }
    }

    return query;
}

QString SearchQueryParser::hasKeyword(SearchQuery::Has has)
{
    switch (has) {
    case SearchQuery::Has::Link:  return QStringLiteral("link");
    case SearchQuery::Has::Image: return QStringLiteral("image");
    case SearchQuery::Has::Video: return QStringLiteral("video");
    case SearchQuery::Has::File:  return QStringLiteral("file");
    case SearchQuery::Has::Embed: return QStringLiteral("embed");
    case SearchQuery::Has::Sound: return QStringLiteral("sound");
    }
    return QString();
}

} // namespace Core
} // namespace Acheron
