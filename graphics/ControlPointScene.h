#ifndef CONTROLPOINTSCENE_H
#define CONTROLPOINTSCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QPointF>

class ControlPoint;
class ImageItem;

class ControlPointScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit ControlPointScene(QObject *parent = nullptr);

    ImageItem *imageItem() const { return m_imageItem; }
    void setImageItem(ImageItem *item);

    QList<ControlPoint *> controlPoints() const;

    ControlPoint *selectedPoint() const { return m_selectedPoint; }
    void setSelectedPoint(ControlPoint *point);

    ControlPoint *addControlPoint(const QPointF &pos);
    void removeControlPoint(ControlPoint *point);
    void clearControlPoints();
    QList<ControlPoint *> pointsInRect(const QRectF &rect) const;
    void removePointsInRect(const QRectF &rect);
    void setRoiRect(const QRectF &rect);
    void clearRoiRect();
    QRectF roiRect() const { return m_currentRoiRect; }
    void setRoiSelectionMode(bool enabled);
    void cancelRoiSelection();

signals:
    void pointSelected(ControlPoint *point);
    void pointAdded(ControlPoint *point);
    void pointRemoved(ControlPoint *point);
    void pointDoubleClicked(ControlPoint *point);
    void pointIdChanged(ControlPoint *point);
    void pointPositionChanged(ControlPoint *point);
    void roiSelected(const QRectF &rect);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    ImageItem *m_imageItem = nullptr;
    ControlPoint *m_selectedPoint = nullptr;
    QPointF m_roiOrigin;
    QPointF m_roiCurrent;
    bool m_isSelectingRoi = false;
    bool m_roiSelectionMode = false;
    QRectF m_currentRoiRect;

    void drawRoiRectangle(QPainter *painter, const QRectF &rect);
    void highlightPointsInRoi();
    void clearRoiPointHighlight();
};

#endif // CONTROLPOINTSCENE_H