#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QRectF>

#include "Core/Presence.hpp"

namespace Acheron {
namespace UI {

// Draws a small platform glyph (monitor / phone / globe / gamepad) stroked in
// the given colour, inside rect. Hand-drawn with primitives so it needs no SVG
// module and scales cleanly at small sizes. Distinct from the filled status dot.
inline void drawPlatformIcon(QPainter *painter, const QRectF &rect, Core::Platform platform,
                             const QColor &color)
{
    if (!color.isValid())
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color);
    pen.setWidthF(qMax(1.0, rect.width() / 9.0));
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    const qreal x = rect.x();
    const qreal y = rect.y();
    const qreal w = rect.width();
    const qreal h = rect.height();
    auto px = [&](qreal fx) { return x + fx * w; };
    auto py = [&](qreal fy) { return y + fy * h; };

    switch (platform) {
    case Core::Platform::Desktop: {
        // Monitor: screen + stand.
        QRectF screen(px(0.12), py(0.16), w * 0.76, h * 0.50);
        painter->drawRoundedRect(screen, w * 0.08, w * 0.08);
        painter->drawLine(QPointF(px(0.5), py(0.66)), QPointF(px(0.5), py(0.80)));
        painter->drawLine(QPointF(px(0.30), py(0.84)), QPointF(px(0.70), py(0.84)));
        break;
    }
    case Core::Platform::Mobile: {
        // Phone: tall rounded body + home dot.
        QRectF body(px(0.30), py(0.10), w * 0.40, h * 0.80);
        painter->drawRoundedRect(body, w * 0.10, w * 0.10);
        painter->save();
        painter->setBrush(color);
        const qreal r = w * 0.045;
        painter->drawEllipse(QPointF(px(0.5), py(0.78)), r, r);
        painter->restore();
        break;
    }
    case Core::Platform::Web: {
        // Globe: circle + equator + meridian.
        QRectF circ(px(0.14), py(0.14), w * 0.72, h * 0.72);
        painter->drawEllipse(circ);
        painter->drawLine(QPointF(px(0.14), py(0.5)), QPointF(px(0.86), py(0.5)));
        QRectF meridian(px(0.36), py(0.14), w * 0.28, h * 0.72);
        painter->drawEllipse(meridian);
        break;
    }
    case Core::Platform::Embedded: {
        // Gamepad: rounded body + two buttons.
        QRectF body(px(0.10), py(0.34), w * 0.80, h * 0.36);
        painter->drawRoundedRect(body, h * 0.18, h * 0.18);
        painter->save();
        painter->setBrush(color);
        const qreal r = w * 0.05;
        painter->drawEllipse(QPointF(px(0.66), py(0.46)), r, r);
        painter->drawEllipse(QPointF(px(0.78), py(0.56)), r, r);
        painter->restore();
        // d-pad
        painter->drawLine(QPointF(px(0.22), py(0.52)), QPointF(px(0.34), py(0.52)));
        painter->drawLine(QPointF(px(0.28), py(0.46)), QPointF(px(0.28), py(0.58)));
        break;
    }
    }

    painter->restore();
}

} // namespace UI
} // namespace Acheron
