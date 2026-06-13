#pragma once

// Compatibility shims for building with both Qt5 and Qt6.
// Include this where Qt6-only APIs are used.

#include <QtGlobal>

// --- qHash seed/return type ---
// Qt6 uses size_t; Qt5 uses uint.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using QHashSeed = size_t;
#else
using QHashSeed = uint;
#endif

// --- qHashMulti (Qt6-only) ---
// Qt5 has no qHashMulti; chain qHash calls manually.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QHashFunctions>
// Qt5 qHash uses uint; the callers below use size_t (Qt6 style).
// Use size_t uniformly; Qt5's uint is implicitly convertible.
template <typename... Args>
inline QHashSeed qHashMulti(QHashSeed seed, const Args &...args)
{
    int dummy[] = { 0, (seed = seed ^ (qHash(args, static_cast<uint>(seed)) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), 0)... };
    (void) dummy;
    return seed;
}
#endif

// --- QKeyCombination (Qt6-only) ---
// In Qt6, QKeySequence::operator[] returns QKeyCombination; .key() extracts the int.
// In Qt5, operator[] returns int directly.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define ACHERON_KEY_INT(seq, idx) static_cast<int>((seq)[(idx)])
#else
#define ACHERON_KEY_INT(seq, idx) (seq)[(idx)].key()
#endif

// --- QKeySequenceEdit::setMaximumSequenceLength (Qt 6.5+) ---
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#define ACHERON_SET_MAX_SEQ_LEN(edit, len) /* not available before Qt 6.5 */
#else
#define ACHERON_SET_MAX_SEQ_LEN(edit, len) (edit)->setMaximumSequenceLength(len)
#endif

// --- beginFilterChange / endFilterChange (Qt 6.10+) ---
// Qt5 / early Qt6: call invalidateFilter() instead.
#if QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
#define ACHERON_BEGIN_FILTER_CHANGE(proxy) /* no-op */
#define ACHERON_END_FILTER_CHANGE(proxy) (proxy)->QSortFilterProxyModel::invalidateFilter()
#else
#define ACHERON_BEGIN_FILTER_CHANGE(proxy) (proxy)->beginFilterChange()
#define ACHERON_END_FILTER_CHANGE(proxy) (proxy)->endFilterChange()
#endif

// --- u"" string literals (Qt6 / C++17 char16_t) ---
// Qt5: use QStringLiteral() or QLatin1String() instead. Most usages can be
// replaced inline; this macro helps the few hot-path cases.
// (Individual files fix u"" usages directly.)
