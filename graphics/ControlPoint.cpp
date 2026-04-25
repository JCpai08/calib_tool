#include "ControlPoint.h"
#include <QPainter>
#include <QDebug>

ControlPoint::ControlPoint(qreal x, qreal y, QGraphicsItem *parent)
    : QAbstractGraphicsShapeItem(parent)
{
    setPos(x, y);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsFocusable);
}

ControlPoint::ControlPoint(const QPointF &pos, QGraphicsItem *parent)
    : ControlPoint(pos.x(), pos.y(), parent)
{}

void ControlPoint::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        update();
    }
}

void ControlPoint::setRadius(qreal r)
{
    m_radius = r;
    m_crossSize = r / 2;
    update();
}

QRectF ControlPoint::boundingRect() const
{
    qreal penWidth = 2.0;
    return QRectF(-m_radius - penWidth, -m_radius - penWidth,
                 m_radius * 2 + penWidth * 2, m_radius * 2 + penWidth * 2);
}

QPainterPath ControlPoint::shape() const
{
    QPainterPath path;
    path.addEllipse(boundingRect());
    return path;
}

void ControlPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QColor color;
    switch (m_state) {
        case Selected: color = m_selectedColor; break;
        case Locked: color = m_lockedColor; break;
        case InRoi: color = m_inRoiColor; break;
        default: color = m_normalColor;
    }

    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(QPen(color, 2.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);

    painter->setPen(QPen(color, 1.5));
    drawCrossHair(painter);
    
    if (m_id > 0) {
        painter->setPen(QPen(Qt::white, 1));
        QFont font = painter->font();
        font.setPixelSize(10);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(QPointF(m_radius + 2, -m_radius - 2), QString::number(m_id));
    }
}

void ControlPoint::drawCrossHair(QPainter *painter)
{
    qreal lineLen = m_radius * 0.8;
    painter->drawLine(QLineF(-lineLen, 0, lineLen, 0));
    painter->drawLine(QLineF(0, -lineLen, 0, lineLen));
}