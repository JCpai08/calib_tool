#include "ZoomableView.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QScrollBar>

ZoomableView::ZoomableView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setBackgroundBrush(Qt::darkGray);
    setViewportUpdateMode(FullViewportUpdate);
}

void ZoomableView::resetZoom()
{
    resetTransform();
    m_zoomFactor = 1.0;
    emit zoomChanged(m_zoomFactor);
}

void ZoomableView::fitInView()
{
    if (scene()) {
        QRectF rect = scene()->sceneRect();
        if (!rect.isEmpty()) {
            const qreal margin = 50;
            QRectF viewRect(0, 0, viewport()->width() - margin, viewport()->height() - margin);
            qreal xScale = viewRect.width() / rect.width();
            qreal yScale = viewRect.height() / rect.height();
            qreal s = qMin(xScale, yScale);
            s = qMin(s, 1.0);

            resetTransform();
            scale(s, s);
            m_zoomFactor = s;

            centerOn(rect.center());
            m_zoomFactor = transform().m11();
        }
    }
}

void ZoomableView::wheelEvent(QWheelEvent *event)
{
    const qreal scaleFactor = 1.15;
    qreal newFactor = m_zoomFactor;

    if (event->angleDelta().y() > 0) {
        newFactor *= scaleFactor;
    } else {
        newFactor /= scaleFactor;
    }

    newFactor = qBound(m_minZoom, newFactor, m_maxZoom);
    if (newFactor != m_zoomFactor) {
        const QPointF scenePos = mapToScene(event->position().toPoint());
        const QPointF viewportPos = event->position();

        qreal factor = newFactor / m_zoomFactor;
        scale(factor, factor);

        m_zoomFactor = newFactor;
        emit zoomChanged(m_zoomFactor);
    }

    event->accept();
}

void ZoomableView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton || (event->modifiers() & Qt::ShiftModifier)) {
        m_isPanning = true;
        m_lastPanPos = event->globalPosition();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void ZoomableView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        panBy(event->globalPosition() - m_lastPanPos);
        m_lastPanPos = event->globalPosition();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void ZoomableView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning && event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void ZoomableView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !m_spacePressed) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void ZoomableView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        m_spacePressed = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void ZoomableView::zoomAt(const QPointF &pos, qreal factor)
{
    QPointF scenePos = mapToScene(pos.toPoint());
    scale(factor, factor);
    QPointF newViewportPos = mapFromScene(scenePos);
    horizontalScrollBar()->setValue(newViewportPos.x() + horizontalScrollBar()->value());
    verticalScrollBar()->setValue(newViewportPos.y() + verticalScrollBar()->value());
}

void ZoomableView::panBy(const QPointF &delta)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
}