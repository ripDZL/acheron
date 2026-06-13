#pragma once

// Custom 64-bit flags class for Discord permissions. Qt5's QFlags is backed by
// int (32-bit) and can't represent permissions above bit 31. This class provides
// the same API surface (testFlag, operator|, fromInt, etc.) so the rest of the
// codebase is unchanged.
//
// On Qt6, the standard Q_DECLARE_FLAGS / QFlags<Permission> is used instead;
// this class is only compiled into Qt5 builds.

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

#include <QHashFunctions>

namespace Acheron {

template <typename Enum>
class Flags64
{
public:
    using Int = quint64;

    constexpr Flags64() noexcept : m_value(0) {}
    constexpr Flags64(Enum flag) noexcept : m_value(static_cast<Int>(flag)) {}
    constexpr explicit Flags64(Int value) noexcept : m_value(value) {}

    static constexpr Flags64 fromInt(Int value) noexcept { return Flags64(value); }

    [[nodiscard]] constexpr Int toInt() const noexcept { return m_value; }
    constexpr explicit operator Int() const noexcept { return m_value; }
    // Allow cast to qint64 for database storage
    constexpr explicit operator qint64() const noexcept { return static_cast<qint64>(m_value); }

    [[nodiscard]] constexpr bool testFlag(Enum flag) const noexcept
    {
        const Int f = static_cast<Int>(flag);
        return (m_value & f) == f && (f != 0 || m_value == Int(0));
    }

    constexpr Flags64 &operator|=(Flags64 other) noexcept { m_value |= other.m_value; return *this; }
    constexpr Flags64 &operator|=(Enum flag) noexcept { m_value |= static_cast<Int>(flag); return *this; }
    constexpr Flags64 &operator&=(Flags64 other) noexcept { m_value &= other.m_value; return *this; }
    constexpr Flags64 &operator&=(Enum flag) noexcept { m_value &= static_cast<Int>(flag); return *this; }
    constexpr Flags64 &operator^=(Flags64 other) noexcept { m_value ^= other.m_value; return *this; }

    [[nodiscard]] constexpr Flags64 operator|(Flags64 other) const noexcept { return Flags64(m_value | other.m_value); }
    [[nodiscard]] constexpr Flags64 operator|(Enum flag) const noexcept { return Flags64(m_value | static_cast<Int>(flag)); }
    [[nodiscard]] constexpr Flags64 operator&(Flags64 other) const noexcept { return Flags64(m_value & other.m_value); }
    [[nodiscard]] constexpr Flags64 operator&(Enum flag) const noexcept { return Flags64(m_value & static_cast<Int>(flag)); }
    [[nodiscard]] constexpr Flags64 operator^(Flags64 other) const noexcept { return Flags64(m_value ^ other.m_value); }
    [[nodiscard]] constexpr Flags64 operator~() const noexcept { return Flags64(~m_value); }

    [[nodiscard]] constexpr bool operator==(Flags64 other) const noexcept { return m_value == other.m_value; }
    [[nodiscard]] constexpr bool operator!=(Flags64 other) const noexcept { return m_value != other.m_value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_value != 0; }
    [[nodiscard]] constexpr bool operator!() const noexcept { return m_value == 0; }

private:
    Int m_value;
};

// Free operator so (Permission | Permission) works without explicit Flags64 construction.
template <typename Enum>
[[nodiscard]] constexpr Flags64<Enum> operator|(Enum a, Enum b) noexcept
{
    return Flags64<Enum>(a) | Flags64<Enum>(b);
}

template <typename Enum>
[[nodiscard]] inline size_t qHash(Flags64<Enum> flags, uint seed = 0) noexcept
{
    return qHash(flags.toInt(), seed);
}

} // namespace Acheron

#endif // QT_VERSION < 6
