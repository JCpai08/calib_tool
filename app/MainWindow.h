#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QString>
#include <QGraphicsScene>
#include <QTableWidget>
#include <QSplitter>
#include <QRect>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QtMath>

#include "graphics/ZoomableView.h"
#include "graphics/ImageItem.h"
#include "graphics/ControlPointScene.h"
#include "core/CalibrationData.h"
#include "core/CircleDetector.h"

class ControlPointPreviewPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ControlPointPreviewPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_label = new QLabel(this);
        m_label->setAlignment(Qt::AlignCenter);
        m_label->setMinimumSize(200, 200);
        m_label->setStyleSheet("border: 1px solid gray; background: black;");

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Preview", this));
        layout->addWidget(m_label);
    }

    void updatePreview(const QPixmap &pixmap, const QPointF &point)
    {
        if (pixmap.isNull()) {
            m_label->setPixmap(QPixmap());
            return;
        }

        int cropSize = 100;
        int viewSize = 200;
        int cx = static_cast<int>(point.x());
        int cy = static_cast<int>(point.y());

        int x1 = std::max(0, cx - cropSize / 2);
        int y1 = std::max(0, cy - cropSize / 2);
        int x2 = std::min(pixmap.width(), cx + cropSize / 2);
        int y2 = std::min(pixmap.height(), cy + cropSize / 2);

        int actualW = x2 - x1;
        int actualH = y2 - y1;

        QPixmap crop = pixmap.copy(x1, y1, actualW, actualH);

        QPixmap padded(cropSize, cropSize);
        padded.fill(Qt::black);
        QPainter painter(&padded);
        painter.drawPixmap((cropSize - actualW) / 2, (cropSize - actualH) / 2, crop);
        painter.end();

        QPainter p(&padded);
        p.setPen(QPen(Qt::red, 1));
        p.drawLine(cropSize / 2, 0, cropSize / 2, cropSize);
        p.drawLine(0, cropSize / 2, cropSize, cropSize / 2);
        p.end();

        m_label->setPixmap(padded.scaled(viewSize, viewSize, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    }

    void clear()
    {
        m_label->setPixmap(QPixmap());
    }

private:
    QLabel *m_label = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onOpenImage();
    void onExportYaml();
    void onExportCsv();
    void onClearPoints();
    void onAutoDetect();
    void onFitInView();
    void onResetZoom();
    void onPointSelected(ControlPoint *point);
    void onPointDoubleClicked(ControlPoint *point);
    void onPointAdded(ControlPoint *point);
    void onPointRemoved(ControlPoint *point);
    void onPointIdChanged(int row, int id);
    void onTableSelectionChanged();
    void onDetectInRoi();
    void onDeleteRoiPoints();
    void onClearRoi();
    void onSelectRoi();
    void onShowConfigDialog();

private:
    void setupUi();
    void setupConnections();
    void createActions();
    void createMenus();
    void createToolbars();
    void updateStatusBar();
    void updateTable();
    void selectTableRow(ControlPoint *point);
    void updateCalibData();
    void applyDetectionConfig();
    void sortPointsByConfidence();
    void updatePreview(ControlPoint *point);

    ZoomableView *m_view = nullptr;
    ControlPointScene *m_scene = nullptr;
    ImageItem *m_imageItem = nullptr;
    CalibrationData m_calibData;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_rightSplitter = nullptr;
    QTableWidget *m_tableWidget = nullptr;
    ControlPointPreviewPanel *m_previewPanel = nullptr;

    QString m_currentFilePath;
    QRect m_currentRoi;

    QAction *m_openImageAction = nullptr;
    QAction *m_exportYamlAction = nullptr;
    QAction *m_exportCsvAction = nullptr;
    QAction *m_clearPointsAction = nullptr;
    QAction *m_autoDetectAction = nullptr;
    QAction *m_fitInViewAction = nullptr;
    QAction *m_resetZoomAction = nullptr;
    QAction *m_exitAction = nullptr;
    QAction *m_detectInRoiAction = nullptr;
    QAction *m_deleteRoiPointsAction = nullptr;
    QAction *m_clearRoiAction = nullptr;
    QAction *m_selectRoiAction = nullptr;
    QAction *m_showConfigAction = nullptr;
    QPushButton *m_roiButton = nullptr;
    CircleDetectionConfig m_detectionConfig;
    QDialog *m_configDialog = nullptr;
    QList<QLineEdit*> m_configEdits;
};

#endif // MAINWINDOW_H