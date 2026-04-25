#ifndef ZOOMABLEVIEW_H
#define ZOOMABLEVIEW_H

#include <QGraphicsView>
#include <QPointF>
#include <QMouseEvent>

class ZoomableView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ZoomableView(QWidget *parent = nullptr);

    void resetZoom();
    void fitInView();

    qreal zoomFactor() const { return m_zoomFactor; }

signals:
    void zoomChanged(qreal factor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void zoomAt(const QPointF &pos, qreal factor);
    void panBy(const QPointF &delta);

    qreal m_zoomFactor = 1.0;
    qreal m_minZoom = 0.1;
    qreal m_maxZoom = 20.0;
    QPointF m_lastPanPos;
    bool m_isPanning = false;
    bool m_spacePressed = false;
};

#endif // ZOOMABLEVIEW_H