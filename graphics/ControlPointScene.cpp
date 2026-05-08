#include "ControlPointScene.h"
#include "ControlPoint.h"
#include "ImageItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QPainter>

ControlPointScene::ControlPointScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setItemIndexMethod(QGraphicsScene::NoIndex);
    setBackgroundBrush(Qt::darkGray);
}

void ControlPointScene::setImageItem(ImageItem *item)
{
    m_imageItem = item;
    if (item) {
        addItem(item);
        setSceneRect(item->boundingRect());
    }
}

QList<ControlPoint *> ControlPointScene::controlPoints() const
{
    QList<ControlPoint *> points;
    for (QGraphicsItem *item : items()) {
        if (ControlPoint *cp = qgraphicsitem_cast<ControlPoint *>(item)) {
            points.append(cp);
        }
    }
    return points;
}

void ControlPointScene::setSelectedPoint(ControlPoint *point)
{
    if (m_selectedPoint) {
        m_selectedPoint->setState(ControlPoint::Normal);
    }
    clearPendingIdInput();
    m_selectedPoint = point;
    if (point) {
        point->setState(ControlPoint::Selected);
        point->setFocus();
        emit pointSelected(point);
    }
}

ControlPoint *ControlPointScene::addControlPoint(const QPointF &pos)
{
    ControlPoint *point = new ControlPoint(pos);
    addItem(point);
    m_selectedPoint = point;
    emit pointAdded(point);
    return point;
}

void ControlPointScene::removeControlPoint(ControlPoint *point)
{
    if (point) {
        removeItem(point);
        if (m_selectedPoint == point) {
            m_selectedPoint = nullptr;
            clearPendingIdInput();
        }
        emit pointRemoved(point);
        delete point;
    }
}

void ControlPointScene::clearControlPoints()
{
    QList<ControlPoint *> points = controlPoints();
    for (ControlPoint *point : points) {
        removeItem(point);
        delete point;
    }
    m_selectedPoint = nullptr;
    clearPendingIdInput();
}

QList<ControlPoint *> ControlPointScene::pointsInRect(const QRectF &rect) const
{
    QList<ControlPoint *> points;
    for (QGraphicsItem *item : items(rect)) {
        if (ControlPoint *cp = qgraphicsitem_cast<ControlPoint *>(item)) {
            points.append(cp);
        }
    }
    return points;
}

void ControlPointScene::removePointsInRect(const QRectF &rect)
{
    QList<ControlPoint *> points = pointsInRect(rect);
    for (ControlPoint *point : points) {
        removeControlPoint(point);
    }
}

void ControlPointScene::setRoiRect(const QRectF &rect)
{
    m_currentRoiRect = rect;
    highlightPointsInRoi();
    update();
}

void ControlPointScene::clearRoiRect()
{
    m_currentRoiRect = QRectF();
    m_isSelectingRoi = false;
    clearRoiPointHighlight();
    update();
}

void ControlPointScene::setRoiSelectionMode(bool enabled)
{
    m_roiSelectionMode = enabled;
    if (!enabled) {
        m_isSelectingRoi = false;
    }
}

void ControlPointScene::cancelRoiSelection()
{
    m_roiSelectionMode = false;
    m_isSelectingRoi = false;
}

void ControlPointScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_roiSelectionMode) {
            m_roiOrigin = event->scenePos();
            m_roiCurrent = m_roiOrigin;
            m_isSelectingRoi = true;
            return;
        }
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        if (ControlPoint *cp = qgraphicsitem_cast<ControlPoint *>(item)) {
            setSelectedPoint(cp);
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void ControlPointScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isSelectingRoi) {
        m_roiCurrent = event->scenePos();
        m_currentRoiRect = QRectF(m_roiOrigin, m_roiCurrent).normalized();
        update();
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void ControlPointScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isSelectingRoi && event->button() == Qt::LeftButton) {
        QPointF endPos = event->scenePos();
        QRectF roiRect = QRectF(m_roiOrigin, endPos).normalized();
        if (roiRect.width() > 5 && roiRect.height() > 5) {
            m_currentRoiRect = roiRect;
            emit roiSelected(roiRect);
        }
        m_isSelectingRoi = false;
        update();
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void ControlPointScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    if (m_isSelectingRoi && m_currentRoiRect.isValid()) {
        drawRoiRectangle(painter, m_currentRoiRect);
    } else if (m_currentRoiRect.isValid()) {
        drawRoiRectangle(painter, m_currentRoiRect);
    }
}

void ControlPointScene::drawRoiRectangle(QPainter *painter, const QRectF &rect)
{
    painter->save();
    painter->setPen(QPen(QColor(255, 255, 0, 180), 2));
    painter->setBrush(QBrush(QColor(255, 255, 0, 30)));
    painter->drawRect(rect);
    painter->restore();
}

void ControlPointScene::highlightPointsInRoi()
{
    QList<ControlPoint *> points = controlPoints();
    for (ControlPoint *cp : points) {
        if (m_currentRoiRect.contains(cp->pos())) {
            cp->setState(ControlPoint::InRoi);
        } else if (cp->state() == ControlPoint::InRoi) {
            cp->setState(ControlPoint::Normal);
        }
    }
}

void ControlPointScene::clearRoiPointHighlight()
{
    QList<ControlPoint *> points = controlPoints();
    for (ControlPoint *cp : points) {
        if (cp->state() == ControlPoint::InRoi) {
            cp->setState(ControlPoint::Normal);
        }
    }
}

void ControlPointScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    if (ControlPoint *cp = qgraphicsitem_cast<ControlPoint *>(item)) {
        emit pointDoubleClicked(cp);
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void ControlPointScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (!m_pendingIdText.isEmpty()) {
            m_pendingIdText.chop(1);
            emit pointIdInputChanged(m_selectedPoint, m_pendingIdText);
        } else if (m_selectedPoint) {
            removeControlPoint(m_selectedPoint);
        }
    } else if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
        if (m_selectedPoint) {
            m_pendingIdText.append(event->text());
            emit pointIdInputChanged(m_selectedPoint, m_pendingIdText);
        }
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        commitPendingIdInput();
    } else if (event->key() == Qt::Key_Escape) {
        clearPendingIdInput();
    } else {
        QGraphicsScene::keyPressEvent(event);
    }
}

void ControlPointScene::clearPendingIdInput()
{
    if (m_pendingIdText.isEmpty()) {
        return;
    }

    ControlPoint *point = m_selectedPoint;
    m_pendingIdText.clear();
    emit pointIdInputChanged(point, m_pendingIdText);
}

void ControlPointScene::commitPendingIdInput()
{
    if (!m_selectedPoint || m_pendingIdText.isEmpty()) {
        return;
    }

    bool ok = false;
    int id = m_pendingIdText.toInt(&ok);
    ControlPoint *point = m_selectedPoint;
    clearPendingIdInput();

    if (ok && id > 0) {
        point->setId(id);
        emit pointIdChanged(point);
    }
}
