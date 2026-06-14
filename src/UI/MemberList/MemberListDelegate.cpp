#include "MemberListDelegate.hpp"

#include <QPainter>
#include <QPainterPath>

#include "MemberListModel.hpp"
#include "Core/MemberListManager.hpp"
#include "Core/Presence.hpp"
#include "Core/PresenceManager.hpp"
#include "UI/PlatformIcons.hpp"

constexpr static int GroupHeight = 22;
constexpr static int MemberHeight = 28;
constexpr static int AvatarSize = 20;
constexpr static int AvatarRadius = 4;
constexpr static int HorizontalPadding = 8;
constexpr static int AvatarTextSpacing = 8;

namespace Acheron {
namespace UI {

namespace {
// Draws a small gold crown glyph within the given rect. Vector-drawn so it
// stays crisp at any DPI and needs no bundled asset.
void drawOwnerCrown(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const qreal x = rect.x();
    const qreal y = rect.y();
    const qreal w = rect.width();
    const qreal h = rect.height();

    // Crown body: a base bar with three points rising to peaks.
    QPainterPath crown;
    const qreal baseTop = y + h * 0.72;
    const qreal bottom = y + h * 0.88;
    crown.moveTo(x + w * 0.04, baseTop);
    crown.lineTo(x + w * 0.04, y + h * 0.30);          // left spire up
    crown.lineTo(x + w * 0.27, y + h * 0.60);          // dip
    crown.lineTo(x + w * 0.50, y + h * 0.18);          // center spire up
    crown.lineTo(x + w * 0.73, y + h * 0.60);          // dip
    crown.lineTo(x + w * 0.96, y + h * 0.30);          // right spire up
    crown.lineTo(x + w * 0.96, baseTop);               // down to base
    crown.closeSubpath();

    const QColor gold(0xF1, 0xC4, 0x0F);
    painter->setPen(Qt::NoPen);
    painter->setBrush(gold);
    painter->drawPath(crown);

    // Base bar under the crown.
    painter->drawRoundedRect(QRectF(x + w * 0.04, baseTop, w * 0.92, bottom - baseTop),
                             1.0, 1.0);
    painter->restore();
}
} // namespace

MemberListDelegate::MemberListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void MemberListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    int itemType = index.data(MemberListModel::ItemTypeRole).toInt();

    if (itemType == static_cast<int>(Core::MemberListItem::Type::Group))
        paintGroup(painter, option, index);
    else if (itemType == static_cast<int>(Core::MemberListItem::Type::Member))
        paintMember(painter, option, index);
    else
        paintPlaceholder(painter, option);

    painter->restore();
}

QSize MemberListDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    int itemType = index.data(MemberListModel::ItemTypeRole).toInt();

    if (itemType == static_cast<int>(Core::MemberListItem::Type::Group))
        return QSize(option.rect.width(), GroupHeight);

    return QSize(option.rect.width(), MemberHeight);
}

void MemberListDelegate::paintGroup(QPainter *painter, const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QString groupName = index.data(MemberListModel::GroupNameRole).toString();
    int groupCount = index.data(MemberListModel::GroupCountRole).toInt();

    // separator except for the first
    if (index.row() > 0) {
        QColor sepColor = option.palette.mid().color();
        sepColor.setAlpha(60);
        painter->setPen(QPen(sepColor, 1));
        painter->drawLine(option.rect.left() + HorizontalPadding,
                          option.rect.top(),
                          option.rect.right() - HorizontalPadding,
                          option.rect.top());
    }

    QString text = groupName.toUpper() + QString::fromUtf8(" \u2014 ") + QString::number(groupCount);

    QFont font = option.font;
    font.setPixelSize(10);
    font.setWeight(QFont::DemiBold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.3);
    painter->setFont(font);

    painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));

    QRect textRect = option.rect.adjusted(HorizontalPadding, 0, -HorizontalPadding, 0);
    textRect.setTop(textRect.top() + 6);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void MemberListDelegate::paintMember(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    if (option.state & QStyle::State_MouseOver) {
        QColor hoverColor = option.palette.highlight().color();
        hoverColor.setAlpha(30);
        painter->fillRect(option.rect.adjusted(HorizontalPadding / 2, 1,
                                               -HorizontalPadding / 2, -1),
                          hoverColor);
    }

    int x = option.rect.left() + HorizontalPadding;
    int centerY = option.rect.top() + (option.rect.height() - AvatarSize) / 2;

    // Presence status dot to the LEFT of the avatar (reserves a fixed column so
    // members stay aligned whether or not a status is known).
    {
        constexpr int StatusColumn = 14;
        constexpr qreal DotRadius = 3.5;
        auto status = static_cast<Core::PresenceStatus>(index.data(MemberListModel::StatusRole).toInt());
        QColor dotColor = Core::presenceDotColor(status);
        if (dotColor.isValid()) {
            QPointF c(x + DotRadius, option.rect.center().y() + 0.5);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(dotColor);
            painter->drawEllipse(c, DotRadius, DotRadius);
            painter->restore();
        }
        x += StatusColumn;
    }

    QPixmap avatar = index.data(MemberListModel::AvatarRole).value<QPixmap>();
    QRect avatarRect(x, centerY, AvatarSize, AvatarSize);

    if (!avatar.isNull()) {
        QPainterPath clipPath;
        clipPath.addRoundedRect(avatarRect, AvatarRadius, AvatarRadius);
        painter->save();
        painter->setClipPath(clipPath);
        painter->drawPixmap(avatarRect, avatar.scaled(AvatarSize, AvatarSize,
                                                      Qt::KeepAspectRatioByExpanding,
                                                      Qt::SmoothTransformation));
        painter->restore();
    } else {
        QColor defaultBg = option.palette.mid().color();
        defaultBg.setAlpha(100);
        painter->setBrush(defaultBg);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(avatarRect, AvatarRadius, AvatarRadius);
    }

    x += AvatarSize + AvatarTextSpacing;

    // Platform indicators, right-aligned; reserve their width before eliding name.
    static const QVector<Core::PlatformStatus> emptyPlatforms;
    const auto &platforms = presenceManager
            ? presenceManager->platformsOf(
                      Core::Snowflake(index.data(MemberListModel::UserIdRole).toULongLong()))
            : emptyPlatforms;
    constexpr int PlatIconSize = 14;
    constexpr int PlatIconGap = 2;
    const int platformsWidth =
            platforms.isEmpty() ? 0 : platforms.size() * (PlatIconSize + PlatIconGap);

    // Owner crown: reserve space to the right of the name (left of platforms).
    const bool isOwner = MemberListModel::showOwnerCrown()
            && index.data(MemberListModel::IsOwnerRole).toBool();
    constexpr int CrownSize = 12;
    constexpr int CrownGap = 4;
    const int crownWidth = isOwner ? (CrownSize + CrownGap) : 0;

    int textWidth = option.rect.right() - x - HorizontalPadding - platformsWidth - crownWidth;
    QRect nameRect(x, option.rect.top(), textWidth, option.rect.height());

    QString displayName = index.data(MemberListModel::UsernameRole).toString();
    QColor roleColor = index.data(MemberListModel::RoleColorRole).value<QColor>();

    QFont font = option.font;
    font.setPixelSize(12);
    font.setWeight(QFont::Medium);
    painter->setFont(font);

    QColor nameColor;
    if (roleColor.isValid())
        nameColor = roleColor;
    else
        nameColor = option.palette.color(QPalette::Text);

    painter->setPen(nameColor);

    QFontMetrics fm(font);
    QString elidedName = fm.elidedText(displayName, Qt::ElideRight, textWidth);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

    // Draw the crown immediately after the (elided) name text.
    if (isOwner) {
        const int nameTextWidth = qMin(fm.horizontalAdvance(elidedName), textWidth);
        const int crownX = x + nameTextWidth + CrownGap;
        const int crownY = option.rect.top() + (option.rect.height() - CrownSize) / 2;
        drawOwnerCrown(painter, QRectF(crownX, crownY, CrownSize, CrownSize));
    }

    if (!platforms.isEmpty()) {
        int iconY = option.rect.top() + (option.rect.height() - PlatIconSize) / 2;
        int ix = option.rect.right() - HorizontalPadding - platformsWidth + PlatIconGap;
        for (const auto &p : platforms) {
            drawPlatformIcon(painter, QRectF(ix, iconY, PlatIconSize, PlatIconSize), p.platform,
                             Core::presenceDotColor(p.status));
            ix += PlatIconSize + PlatIconGap;
        }
    }
}

void MemberListDelegate::paintPlaceholder(QPainter *painter,
                                          const QStyleOptionViewItem &option) const
{
    QColor placeholderColor = option.palette.mid().color();
    placeholderColor.setAlpha(40);
    painter->setPen(Qt::NoPen);
    painter->setBrush(placeholderColor);

    int x = option.rect.left() + HorizontalPadding;
    int centerY = option.rect.top() + (option.rect.height() - AvatarSize) / 2;

    painter->drawRoundedRect(QRect(x, centerY, AvatarSize, AvatarSize),
                             AvatarRadius, AvatarRadius);

    x += AvatarSize + AvatarTextSpacing;
    int nameWidth = qMin(80, option.rect.right() - x - HorizontalPadding);
    int nameHeight = 10;
    int nameY = option.rect.top() + (option.rect.height() - nameHeight) / 2;
    painter->drawRoundedRect(QRect(x, nameY, nameWidth, nameHeight), 3, 3);
}

} // namespace UI
} // namespace Acheron
