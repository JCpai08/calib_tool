#ifndef MAGNIFIERPOPUP_H
#define MAGNIFIERPOPUP_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPoint>
#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ControlPoint;

class MagnifierPopup : public QDialog
{
    Q_OBJECT

public:
    explicit MagnifierPopup(QWidget *parent = nullptr);
    ~MagnifierPopup();

    void showAtPoint(ControlPoint *point, const QPixmap &sourcePixmap);

signals:
    void pointIdCommitted(ControlPoint *point);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onIdEditingFinished();

private:
    void updateMagnifier();

    QLabel m_imageLabel;
    QLineEdit m_idEdit;
    QLabel m_idLabel;
    QLabel m_instructionsLabel;

    int m_magnification = 3;
    int m_viewSize = 400;
    QPixmap m_sourcePixmap;
    ControlPoint *m_point = nullptr;
    bool m_isDragging = false;
    QPoint m_dragStartPos;
    QPointF m_originalPos;
    QPointF m_crosshairPos;
};

#endif // MAGNIFIERPOPUP_H