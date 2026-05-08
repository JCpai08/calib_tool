#include "MagnifierPopup.h"
#include "ControlPoint.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QLabel>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>

MagnifierPopup::MagnifierPopup(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    QWidget *container = new QWidget(this);
    container->setStyleSheet("background: black; border: 2px solid red; border-radius: 4px;");

    m_imageLabel.setFixedSize(m_viewSize, m_viewSize);
    m_imageLabel.setScaledContents(false);
    m_imageLabel.setAlignment(Qt::AlignCenter);
    m_imageLabel.setStyleSheet("background: black;");
    m_imageLabel.setFocusPolicy(Qt::NoFocus);

    m_imageLabel.installEventFilter(this);

    QHBoxLayout *idLayout = new QHBoxLayout();
    m_idLabel.setText("ID:");
    m_idLabel.setStyleSheet("color: white; background: transparent;");
    m_idEdit.setFixedWidth(60);
    m_idEdit.setStyleSheet("background: white; border: 1px solid gray; color: black; padding: 2px;");
    idLayout->addWidget(&m_idLabel);
    idLayout->addWidget(&m_idEdit);
    idLayout->addStretch();

    m_instructionsLabel.setText("Arrow keys: move crosshair  |  Enter: confirm  |  Esc: cancel");
    m_instructionsLabel.setStyleSheet("color: #aaaaaa; background: transparent; font-size: 10px;");

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->addWidget(&m_imageLabel);
    layout->addLayout(idLayout);
    layout->addWidget(&m_instructionsLabel);
    layout->setContentsMargins(4, 4, 4, 4);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setFixedSize(m_viewSize + 8, m_viewSize + 60);

    m_idEdit.installEventFilter(this);

    connect(&m_idEdit, &QLineEdit::editingFinished, this, &MagnifierPopup::onIdEditingFinished);
}

MagnifierPopup::~MagnifierPopup()
{}

void MagnifierPopup::showAtPoint(ControlPoint *point, const QPixmap &sourcePixmap)
{
    m_point = point;
    m_sourcePixmap = sourcePixmap;

    if (m_sourcePixmap.isNull() || !m_point) {
        return;
    }

    m_originalPos = m_point->pos();
    m_crosshairPos = m_originalPos;

    if (m_point->hasId()) {
        m_idEdit.setText(QString::number(m_point->id()));
    } else {
        m_idEdit.setText("");
    }

    int screenX = static_cast<int>(m_point->pos().x());
    int screenY = static_cast<int>(m_point->pos().y());

    // 1. 必须获取“当前点击点”所在的屏幕，而不是主屏幕
    QPoint globalPos = point->pos().toPoint(); // 确保这是全局坐标，如果不是请 mapToGlobal
    QScreen *targetScreen = QGuiApplication::screenAt(globalPos);
    if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();

    if (targetScreen) {
        QRect screenGeom = targetScreen->geometry();
        int margin = 20; 

        int popupX = screenGeom.right() - this->width() - margin;
        int popupY = screenGeom.top() + margin;
        // 2. 初始尝试在右侧显示
        // int popupX = globalPos.x() + 50;
        // int popupY = globalPos.y() - m_viewSize / 2;
        // 3. 检查右边界：如果右侧放不下，就弹到左侧
        // 注意：width() 在窗口还没 show 之前可能不准确，建议给个预估值或提前 adjustSize()
        // if (popupX + this->width() > screenGeom.right()) {
        //     popupX = globalPos.x() - this->width() - 50;
        // }
        // // 4. 检查左边界：防止反向弹射后又超出了左边缘
        // if (popupX < screenGeom.left()) {
        //     popupX = screenGeom.left() + 10;
        // }
        // // 5. 检查上下边界
        // if (popupY < screenGeom.top()) {
        //     popupY = screenGeom.top() + 10;
        // }
        // if (popupY + this->height() > screenGeom.bottom()) {
        //     popupY = screenGeom.bottom() - this->height() - 10;
        // }

        move(popupX, popupY);
    }

    show();
    raise();
    setFocus();
    m_idEdit.clearFocus();

    updateMagnifier();
}

bool MagnifierPopup::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == &m_idEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                onIdEditingFinished();
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void MagnifierPopup::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !m_idEdit.hasFocus()) {
        m_idEdit.setFocus(Qt::ShortcutFocusReason);
        m_idEdit.selectAll();
        event->accept();
        return;
    }

    qreal step = 1.0;
    if (event->modifiers() & Qt::ShiftModifier) {
        step = 5.0;
    }

    bool moved = false;
    switch (event->key()) {
        case Qt::Key_Left:
            m_crosshairPos.setX(m_crosshairPos.x() - step);
            moved = true;
            break;
        case Qt::Key_Right:
            m_crosshairPos.setX(m_crosshairPos.x() + step);
            moved = true;
            break;
        case Qt::Key_Up:
            m_crosshairPos.setY(m_crosshairPos.y() - step);
            moved = true;
            break;
        case Qt::Key_Down:
            m_crosshairPos.setY(m_crosshairPos.y() + step);
            moved = true;
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            commitCurrentPosition();
            hide();
            return;
        case Qt::Key_Escape:
            if (m_point) {
                m_point->setPos(m_originalPos);
            }
            hide();
            return;
        default:
            QDialog::keyPressEvent(event);
            return;
    }

    if (moved) {
        m_crosshairPos.setX(qMax(0.0, qMin(m_crosshairPos.x(), static_cast<qreal>(m_sourcePixmap.width()))));
        m_crosshairPos.setY(qMax(0.0, qMin(m_crosshairPos.y(), static_cast<qreal>(m_sourcePixmap.height()))));
        updateMagnifier();
        event->accept();
    }
}

void MagnifierPopup::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_imageLabel.geometry().contains(event->pos())) {
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint();
        m_originalPos = m_crosshairPos;
    }
    QDialog::mousePressEvent(event);
}

void MagnifierPopup::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && m_point) {
        QPoint delta = event->globalPosition().toPoint() - m_dragStartPos;
        m_crosshairPos = m_originalPos + QPointF(delta.x() / m_magnification, delta.y() / m_magnification);

        m_crosshairPos.setX(qMax(0.0, qMin(m_crosshairPos.x(), static_cast<qreal>(m_sourcePixmap.width()))));
        m_crosshairPos.setY(qMax(0.0, qMin(m_crosshairPos.y(), static_cast<qreal>(m_sourcePixmap.height()))));

        updateMagnifier();
    }
    QDialog::mouseMoveEvent(event);
}

void MagnifierPopup::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging && m_point) {
            commitCurrentPosition();
        }
        m_isDragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

void MagnifierPopup::onIdEditingFinished()
{
    if (m_point) {
        commitCurrentPosition();

        bool ok;
        int id = m_idEdit.text().toInt(&ok);
        if (ok && id > 0) {
            m_point->setId(id);
            emit pointIdCommitted(m_point);
            hide();
        } else {
            m_point->setId(-1);
            m_idEdit.setText("");
        }
    }
}

void MagnifierPopup::commitCurrentPosition()
{
    if (!m_point || m_point->pos() == m_crosshairPos) {
        return;
    }

    m_point->setPos(m_crosshairPos);
    emit pointPositionCommitted(m_point);
}

void MagnifierPopup::updateMagnifier()
{
    if (m_sourcePixmap.isNull() || !m_point) {
        return;
    }

    QPointF centerPos = m_crosshairPos;
    int halfSize = m_viewSize / (2 * m_magnification);

    int x = static_cast<int>(centerPos.x()) - halfSize;
    int y = static_cast<int>(centerPos.y()) - halfSize;

    x = qMax(0, x);
    y = qMax(0, y);

    QPixmap cropped = m_sourcePixmap.copy(x, y, halfSize * 2, halfSize * 2);
    QPixmap scaled = cropped.scaled(m_viewSize, m_viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int w = scaled.width();
    int h = scaled.height();

    QPainter painter(&scaled);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen redPen(Qt::red, 1);
    painter.setPen(redPen);
    painter.drawLine(0, h/2, w, h/2);
    painter.drawLine(w/2, 0, w/2, h);

    QPen greenPen(Qt::green, 2);
    painter.setPen(greenPen);
    int centerX = w/2;
    int centerY = h/2;
    int dotSize = 4;
    painter.drawLine(centerX - dotSize, centerY, centerX + dotSize, centerY);
    painter.drawLine(centerX, centerY - dotSize, centerX, centerY + dotSize);

    painter.end();

    m_imageLabel.setPixmap(scaled);
}
