#ifndef CONTROLPOINT_H
#define CONTROLPOINT_H

#include <QAbstractGraphicsShapeItem>
#include <QGraphicsItem>
#include <QColor>
#include <QPen>

class ControlPoint : public QAbstractGraphicsShapeItem
{
public:
    enum State { Normal, Selected, Locked, InRoi };

    explicit ControlPoint(qreal x, qreal y, QGraphicsItem *parent = nullptr);
    explicit ControlPoint(const QPointF &pos, QGraphicsItem *parent = nullptr);

    int id() const { return m_id; }
    void setId(int id);

    bool hasId() const { return m_id > 0; }

    State state() const { return m_state; }
    void setState(State state);

    qreal radius() const { return m_radius; }
    void setRadius(qreal r);

    qreal confidence() const { return m_confidence; }
    void setConfidence(qreal c) { m_confidence = c; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

private:
    void drawCrossHair(QPainter *painter);

    int m_id = -1;
    State m_state = Normal;
    qreal m_radius = 12.0;
    qreal m_crossSize = 6.0;
    qreal m_confidence = 0.5;

    QColor m_normalColor = QColor(0, 200, 0);
    QColor m_selectedColor = QColor(255, 0, 0);
    QColor m_lockedColor = QColor(128, 128, 128);
    QColor m_inRoiColor = QColor(0, 150, 255);
};

#endif // CONTROLPOINT_H
