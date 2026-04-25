#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsPixmapItem>

class ImageItem : public QGraphicsPixmapItem
{
public:
    explicit ImageItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr);

    void setImage(const QString &filePath);
    void setImage(const QPixmap &pixmap);

    QRectF boundingRect() const override;

    enum { Type = UserType + 10 };
    int type() const override { return Type; }

private:
    QPixmap m_originalPixmap;
};

#endif // IMAGEITEM_H