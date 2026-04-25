#include "ImageItem.h"
#include <QPainter>

ImageItem::ImageItem(const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsPixmapItem(pixmap, parent), m_originalPixmap(pixmap)
{
    setTransformationMode(Qt::SmoothTransformation);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
}

void ImageItem::setImage(const QString &filePath)
{
    QPixmap pixmap(filePath);
    if (!pixmap.isNull()) {
        m_originalPixmap = pixmap;
        setPixmap(pixmap);
    }
}

void ImageItem::setImage(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    setPixmap(pixmap);
}

QRectF ImageItem::boundingRect() const
{
    return QGraphicsPixmapItem::boundingRect();
}