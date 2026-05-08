#include "MainWindow.h"
#include "graphics/ControlPoint.h"
#include "graphics/MagnifierPopup.h"
#include "core/CircleDetector.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QHeaderView>
#include <QKeyEvent>
#include <QFormLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QFileInfo>
#include <QTimer>

#include <opencv2/imgcodecs.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    createActions();
    createMenus();
    createToolbars();
    setupConnections();
}

MainWindow::~MainWindow()
{}

void MainWindow::setupUi()
{
    m_view = new ZoomableView(this);
    m_view->setMinimumSize(480, 360);
    m_scene = new ControlPointScene(this);
    m_view->setScene(m_scene);

    m_imageItem = new ImageItem(QPixmap());
    m_scene->setImageItem(m_imageItem);
    // m_imageItem->setVisible(false);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(4);
    m_tableWidget->setHorizontalHeaderLabels({"ID", "X", "Y", "Conf"});
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed);
    m_tableWidget->setMinimumWidth(200);

    m_previewPanel = new ControlPointPreviewPanel(this);

    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    m_rightSplitter->addWidget(m_previewPanel);
    m_rightSplitter->addWidget(m_tableWidget);
    m_rightSplitter->setStretchFactor(0, 0);
    m_rightSplitter->setStretchFactor(1, 1);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(m_rightSplitter);
    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({960, 320});

    setCentralWidget(m_splitter);
    statusBar()->showMessage("Ready");
}

void MainWindow::createActions()
{
    m_openImageAction = new QAction(tr("Open Image..."), this);
    m_openImageAction->setShortcut(QKeySequence::Open);
    m_openImageAction->setStatusTip(tr("Open an image file"));

    m_importPointsAction = new QAction(tr("Import Points..."), this);
    m_importPointsAction->setStatusTip(tr("Import control points from YAML or CSV"));

    m_exportYamlAction = new QAction(tr("Export YAML..."), this);
    m_exportYamlAction->setShortcut(QKeySequence::Save);
    m_exportYamlAction->setStatusTip(tr("Export to YAML file"));
    m_exportYamlAction->setEnabled(false);

    m_exportCsvAction = new QAction(tr("Export CSV..."), this);
    m_exportCsvAction->setShortcut(QKeySequence::SaveAs);
    m_exportCsvAction->setStatusTip(tr("Export to CSV file"));
    m_exportCsvAction->setEnabled(false);

    m_clearPointsAction = new QAction(tr("Clear Points"), this);
    m_clearPointsAction->setStatusTip(tr("Clear all control points"));
    m_clearPointsAction->setEnabled(false);

    m_autoDetectAction = new QAction(tr("Auto Detect"), this);
    m_autoDetectAction->setStatusTip(tr("Automatically detect control points"));
    m_autoDetectAction->setEnabled(false);

    m_fitInViewAction = new QAction(tr("Fit in View"), this);
    m_fitInViewAction->setShortcut(tr("F"));
    m_fitInViewAction->setStatusTip(tr("Fit image in view"));

    m_resetZoomAction = new QAction(tr("Reset Zoom"), this);
    m_resetZoomAction->setShortcut(tr("Ctrl+0"));
    m_resetZoomAction->setStatusTip(tr("Reset zoom to 100%"));

    m_detectInRoiAction = new QAction(tr("Detect in ROI"), this);
    m_detectInRoiAction->setShortcut(tr("D"));
    m_detectInRoiAction->setStatusTip(tr("Run detection in ROI"));
    m_detectInRoiAction->setEnabled(false);

    m_deleteRoiPointsAction = new QAction(tr("Delete ROI Points"), this);
    m_deleteRoiPointsAction->setShortcut(tr("X"));
    m_deleteRoiPointsAction->setStatusTip(tr("Delete points in ROI (press X)"));
    m_deleteRoiPointsAction->setEnabled(false);

    m_clearRoiAction = new QAction(tr("Clear ROI"), this);
    m_clearRoiAction->setStatusTip(tr("Clear ROI selection"));
    m_clearRoiAction->setEnabled(false);

    m_selectRoiAction = new QAction(tr("Select ROI"), this);
    m_selectRoiAction->setShortcut(tr("R"));
    m_selectRoiAction->setStatusTip(tr("Toggle ROI selection mode (press R)"));
    m_selectRoiAction->setCheckable(true);
    m_selectRoiAction->setEnabled(false);

    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit application"));

    m_showConfigAction = new QAction(tr("Detection Config..."), this);
    m_showConfigAction->setStatusTip(tr("Open detection parameter configuration"));
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(m_openImageAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_importPointsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exportYamlAction);
    fileMenu->addAction(m_exportCsvAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *editMenu = menuBar()->addMenu(tr("Edit"));
    editMenu->addAction(m_clearPointsAction);
    editMenu->addSeparator();
    editMenu->addAction(m_deleteRoiPointsAction);
    editMenu->addAction(m_clearRoiAction);

    QMenu *viewMenu = menuBar()->addMenu(tr("View"));
    viewMenu->addAction(m_fitInViewAction);
    viewMenu->addAction(m_resetZoomAction);

    QMenu *toolsMenu = menuBar()->addMenu(tr("Tools"));
    toolsMenu->addAction(m_showConfigAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_autoDetectAction);
    toolsMenu->addAction(m_selectRoiAction);
    toolsMenu->addAction(m_detectInRoiAction);
}

void MainWindow::createToolbars()
{
    QToolBar *toolbar = addToolBar(tr("Main"));
    toolbar->addAction(m_openImageAction);
    toolbar->addAction(m_importPointsAction);
    toolbar->addSeparator();
    toolbar->addAction(m_autoDetectAction);

    QWidget *spacer1 = new QWidget();
    spacer1->setFixedWidth(10);
    toolbar->addWidget(spacer1);

    m_roiButton = new QPushButton("ROI", this);
    m_roiButton->setCheckable(true);
    m_roiButton->setFixedWidth(60);
    m_roiButton->setEnabled(false);
    m_roiButton->setStyleSheet(
        "QPushButton { font-weight: bold; background-color: #f0f0f0; color: gray;}"
        "QPushButton:checked { background-color: #4caf50; color: white; font-weight: bold; }"
        "QPushButton:disabled { color: #a0a0a0; }"
    );
    connect(m_roiButton, &QPushButton::clicked, this, [this](bool checked) {
        m_selectRoiAction->setChecked(checked);
        m_scene->setRoiSelectionMode(checked);
        if (!checked) {
            m_scene->cancelRoiSelection();
        }
    });
    toolbar->addWidget(m_roiButton);

    QWidget *spacer2 = new QWidget();
    spacer2->setFixedWidth(10);
    toolbar->addWidget(spacer2);

    toolbar->addAction(m_detectInRoiAction);
    toolbar->addAction(m_clearPointsAction);
    toolbar->addSeparator();
    toolbar->addAction(m_fitInViewAction);
    toolbar->addAction(m_resetZoomAction);
}

void MainWindow::setupConnections()
{
    connect(m_openImageAction, &QAction::triggered, this, &MainWindow::onOpenImage);
    connect(m_importPointsAction, &QAction::triggered, this, &MainWindow::onImportPoints);
    connect(m_exportYamlAction, &QAction::triggered, this, &MainWindow::onExportYaml);
    connect(m_exportCsvAction, &QAction::triggered, this, &MainWindow::onExportCsv);
    connect(m_clearPointsAction, &QAction::triggered, this, &MainWindow::onClearPoints);
    connect(m_autoDetectAction, &QAction::triggered, this, &MainWindow::onAutoDetect);
    connect(m_fitInViewAction, &QAction::triggered, this, &MainWindow::onFitInView);
    connect(m_resetZoomAction, &QAction::triggered, this, &MainWindow::onResetZoom);
    connect(m_detectInRoiAction, &QAction::triggered, this, &MainWindow::onDetectInRoi);
    connect(m_deleteRoiPointsAction, &QAction::triggered, this, &MainWindow::onDeleteRoiPoints);
    connect(m_clearRoiAction, &QAction::triggered, this, &MainWindow::onClearRoi);
    connect(m_selectRoiAction, &QAction::triggered, this, &MainWindow::onSelectRoi);
    connect(m_exitAction, &QAction::triggered, qApp, &QApplication::quit);

    connect(m_showConfigAction, &QAction::triggered, this, &MainWindow::onShowConfigDialog);

    connect(m_scene, &ControlPointScene::pointSelected, this, &MainWindow::onPointSelected);
    connect(m_scene, &ControlPointScene::pointDoubleClicked, this, &MainWindow::onPointDoubleClicked);
    connect(m_scene, &ControlPointScene::pointAdded, this, &MainWindow::onPointAdded);
    connect(m_scene, &ControlPointScene::pointRemoved, this, &MainWindow::onPointRemoved);
    connect(m_scene, &ControlPointScene::pointIdChanged, this, [this](ControlPoint *point) {
        updateTable();
        updateCalibData();
        selectTableRow(point);
        statusBar()->showMessage(tr("Point ID set to %1").arg(point->id()));
    });
    connect(m_scene, &ControlPointScene::pointIdInputChanged, this, [this](ControlPoint *point, const QString &text) {
        Q_UNUSED(point);
        if (text.isEmpty()) {
            updateStatusBar();
        } else {
            statusBar()->showMessage(tr("Point ID input: %1 (press Enter to confirm)").arg(text));
        }
    });
    connect(m_scene, &ControlPointScene::roiSelected, this, [this](const QRectF &rect) {
        m_currentRoi = rect.toRect();
        m_scene->setRoiRect(rect);
        m_detectInRoiAction->setEnabled(true);
        m_deleteRoiPointsAction->setEnabled(true);
        m_clearRoiAction->setEnabled(true);
        updateStatusBar();
    });

    connect(m_tableWidget, QOverload<int, int>::of(&QTableWidget::cellChanged), 
            this, &MainWindow::onTableCellChanged);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &MainWindow::onTableSelectionChanged);

    connect(m_view, &ZoomableView::zoomChanged, this, &MainWindow::updateStatusBar);
}

void MainWindow::onOpenImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Open Image"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.tiff *.JPG *.JPEG *.BMP *.TIFF)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_currentFilePath = filePath;
    m_imageItem->setImage(filePath);
    m_imageItem->setVisible(true);
    m_scene->setSceneRect(m_imageItem->boundingRect());
    m_scene->update();

    m_view->fitInView();
    QTimer::singleShot(0, this, [this]() {
        if (m_view->width() < 480) {
            m_splitter->setSizes({960, 320});
        }
        m_view->fitInView();
        m_view->viewport()->update();
    });
    m_autoDetectAction->setEnabled(true);
    m_showConfigAction->setEnabled(true);
    m_selectRoiAction->setEnabled(true);
    if (m_roiButton) m_roiButton->setEnabled(true);

    m_calibData.setImageSize(m_imageItem->pixmap().width(),
                           m_imageItem->pixmap().height());

    m_tableWidget->setRowCount(0); // 清空旧数据
    m_previewPanel->clear();      // 清空预览面板   
    m_clearPointsAction->trigger(); // 清空控制点

    statusBar()->showMessage(tr("Loaded: %1 (%2x%3)")
        .arg(filePath)
        .arg(m_imageItem->pixmap().width())
        .arg(m_imageItem->pixmap().height()));
}

void MainWindow::onImportPoints()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Import Points"), QString(),
        tr("Point files (*.yaml *.yml *.csv);;YAML files (*.yaml *.yml);;CSV files (*.csv)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (!m_scene->controlPoints().isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Import Points"),
            tr("Importing points will replace the current control points. Continue?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    CalibrationData importedData;
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    bool ok = false;
    if (suffix == "csv") {
        ok = importedData.importCsv(filePath);
    } else if (suffix == "yaml" || suffix == "yml") {
        ok = importedData.importYaml(filePath);
    }

    if (!ok) {
        QMessageBox::warning(this, tr("Import Points"), tr("Failed to import points from the selected file."));
        return;
    }

    if (importedData.imageWidth() <= 0 || importedData.imageHeight() <= 0) {
        importedData.setImageSize(m_calibData.imageWidth(), m_calibData.imageHeight());
    }

    m_scene->clearControlPoints();
    for (const CalibrationPoint &point : importedData.points()) {
        ControlPoint *cp = m_scene->addControlPoint(QPointF(point.x, point.y));
        cp->setId(point.id);
        cp->setConfidence(0.0);
    }
    m_scene->clearSelection();

    m_calibData = importedData;
    updateTable();

    const int count = importedData.pointCount();
    m_clearPointsAction->setEnabled(count > 0);
    m_exportYamlAction->setEnabled(count > 0);
    m_exportCsvAction->setEnabled(count > 0);
    statusBar()->showMessage(tr("Imported %1 points").arg(count));
}

void MainWindow::onExportYaml()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export YAML"), QString(),
        tr("YAML files (*.yaml *.yml)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (m_calibData.exportYaml(filePath)) {
        QMessageBox::information(this, tr("Export"), tr("Exported successfully!"));
    } else {
        QMessageBox::warning(this, tr("Export"), tr("Export failed!"));
    }
}

void MainWindow::onExportCsv()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export CSV"), QString(),
        tr("CSV files (*.csv)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (m_calibData.exportCsv(filePath)) {
        QMessageBox::information(this, tr("Export"), tr("Exported successfully!"));
    } else {
        QMessageBox::warning(this, tr("Export"), tr("Export failed!"));
    }
}

void MainWindow::onClearPoints()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Clear Points"),
        tr("Are you sure you want to clear all control points?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_scene->clearControlPoints();
        m_calibData.clear();
        updateTable();
        m_exportYamlAction->setEnabled(false);
        m_exportCsvAction->setEnabled(false);
        m_clearPointsAction->setEnabled(false);
    }
}

void MainWindow::onAutoDetect()
{
    if (m_imageItem->pixmap().isNull()) {
        return;
    }

    cv::Mat image = cv::imread(m_currentFilePath.toStdString());
    if (image.empty()) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load image!"));
        return;
    }

    CircleDetector detector(m_detectionConfig);
    std::vector<CirclePoint> points = detector.detect(image);

    m_scene->clearControlPoints();
    m_calibData.clear();

    for (const auto &pt : points) {
        ControlPoint *cp = m_scene->addControlPoint(QPointF(pt.x, pt.y));
        cp->setId(-1);
        cp->setConfidence(pt.confidence);
    }

    m_scene->clearSelection();
    updateTable();

    int count = static_cast<int>(points.size());
    statusBar()->showMessage(tr("Detected %1 points").arg(count));
    m_clearPointsAction->setEnabled(count > 0);
    m_exportYamlAction->setEnabled(count > 0);
    m_exportCsvAction->setEnabled(count > 0);
}

void MainWindow::onFitInView()
{
    m_view->fitInView();
}

void MainWindow::onResetZoom()
{
    m_view->resetZoom();
}

void MainWindow::onPointSelected(ControlPoint *point)
{
    if (point) {
        selectTableRow(point);
        updatePreview(point);
    }
}

void MainWindow::onPointDoubleClicked(ControlPoint *point)
{
    static MagnifierPopup *popup = new MagnifierPopup(this);
    static bool connected = false;
    if (!connected) {
        connect(popup, &MagnifierPopup::pointIdCommitted, this, [this](ControlPoint *committedPoint) {
            Q_UNUSED(committedPoint);
            updateTable();
            updateCalibData();
        });
        connect(popup, &MagnifierPopup::pointPositionCommitted, this, [this](ControlPoint *committedPoint) {
            updateTable();
            updateCalibData();
            selectTableRow(committedPoint);
            updatePreview(committedPoint);
        });
        connected = true;
    }
    popup->showAtPoint(point, m_imageItem->pixmap());
}

void MainWindow::onPointAdded(ControlPoint *point)
{
    updateTable();
}

void MainWindow::onPointRemoved(ControlPoint *point)
{
    updateTable();
}

void MainWindow::onTableCellChanged(int row, int column)
{
    // Only handle ID column (column 0) changes
    if (column != 0) {
        return;
    }

    QTableWidgetItem *item = m_tableWidget->item(row, 0);
    if (!item) {
        return;
    }

    ControlPoint *point = static_cast<ControlPoint*>(item->data(Qt::UserRole).value<void*>());
    if (!point) {
        return;
    }

    // Parse the ID value from the cell text
    bool ok;
    int newId = item->text().toInt(&ok);
    
    // Validate: ID must be positive
    if (!ok || newId <= 0) {
        point->setId(-1);
        updateCalibData();
        updateTable();
        return;
    }

    // Apply the ID to the point
    point->setId(newId);

    // Update calibration data and keep the table aligned with the scene order.
    updateCalibData();
    updateTable();
}
void MainWindow::onTableSelectionChanged()
{
    int currentRow = m_tableWidget->currentRow();
    if (currentRow >= 0) {
        QTableWidgetItem *item = m_tableWidget->item(currentRow, 0);
        if (item) {
            ControlPoint *cp = static_cast<ControlPoint*>(item->data(Qt::UserRole).value<void*>());
            if (cp) {
                m_scene->setSelectedPoint(cp);
                updatePreview(cp);
            }
        }
    }
}

void MainWindow::onDetectInRoi()
{
    if (m_imageItem->pixmap().isNull() || m_currentRoi.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select an ROI first!"));
        return;
    }

    cv::Mat image = cv::imread(m_currentFilePath.toStdString());
    if (image.empty()) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load image!"));
        return;
    }

    cv::Rect roi(m_currentRoi.x(), m_currentRoi.y(), m_currentRoi.width(), m_currentRoi.height());
    CircleDetector detector(m_detectionConfig);
    std::vector<CirclePoint> points = detector.detectInRoi(image, roi);

    for (const auto &pt : points) {
        ControlPoint *cp = m_scene->addControlPoint(QPointF(pt.x, pt.y));
        cp->setId(-1);
        cp->setConfidence(pt.confidence);
    }

    updateTable();

    int count = static_cast<int>(points.size());
    statusBar()->showMessage(tr("Detected %1 points in ROI").arg(count));
    m_clearPointsAction->setEnabled(m_scene->controlPoints().size() > 0);
    m_exportYamlAction->setEnabled(m_scene->controlPoints().size() > 0);
    m_exportCsvAction->setEnabled(m_scene->controlPoints().size() > 0);
}

void MainWindow::onDeleteRoiPoints()
{
    if (m_currentRoi.isNull()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select an ROI first!"));
        return;
    }

    m_scene->removePointsInRect(m_currentRoi);
    updateTable();
    updateCalibData();

    int count = m_scene->controlPoints().size();
    statusBar()->showMessage(tr("%1 points remaining").arg(count));
    m_clearPointsAction->setEnabled(count > 0);
    m_exportYamlAction->setEnabled(count > 0);
    m_exportCsvAction->setEnabled(count > 0);
}

void MainWindow::onClearRoi()
{
    m_currentRoi = QRect();
    m_scene->clearRoiRect();
    m_detectInRoiAction->setEnabled(false);
    m_deleteRoiPointsAction->setEnabled(false);
    m_clearRoiAction->setEnabled(false);
    m_selectRoiAction->setChecked(false);
    if (m_roiButton) m_roiButton->setChecked(false);
    statusBar()->showMessage("ROI cleared");
}

void MainWindow::onSelectRoi()
{
    bool checked = m_selectRoiAction->isChecked();
    m_scene->setRoiSelectionMode(checked);
    if (!checked) {
        m_scene->cancelRoiSelection();
    }
    if (m_roiButton) m_roiButton->setChecked(checked);
}

void MainWindow::updateStatusBar()
{
    int pointCount = m_scene->controlPoints().size();
    QString roiInfo = m_currentRoi.isNull() ? "" : QString(" | ROI: %1").arg(m_currentRoi.width() > 0 ? QString("%1,%2,%3,%4").arg(m_currentRoi.x()).arg(m_currentRoi.y()).arg(m_currentRoi.width()).arg(m_currentRoi.height()) : "none");
    QString msg = QString("Points: %1 | Zoom: %2x%3")
        .arg(pointCount)
        .arg(m_view->zoomFactor(), 0, 'f', 2)
        .arg(roiInfo);
    statusBar()->showMessage(msg);
}

void MainWindow::updateTable()
{
    QList<ControlPoint *> points = m_scene->controlPoints();

    std::sort(points.begin(), points.end(), [](ControlPoint *a, ControlPoint *b) {
        return a->confidence() < b->confidence();
    });

    m_tableWidget->blockSignals(true);
    m_tableWidget->setRowCount(points.size());

    for (int i = 0; i < points.size(); ++i) {
        ControlPoint *cp = points[i];

        QTableWidgetItem *idItem = new QTableWidgetItem(cp->hasId() ? QString::number(cp->id()) : "");
        idItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(cp)));
        m_tableWidget->setItem(i, 0, idItem);

        m_tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(cp->pos().x(), 'f', 1)));
        m_tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(cp->pos().y(), 'f', 1)));
        m_tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(cp->confidence(), 'f', 2)));
    }

    m_tableWidget->blockSignals(false);
}

void MainWindow::selectTableRow(ControlPoint *point)
{
    for (int i = 0; i < m_tableWidget->rowCount(); ++i) {
        QTableWidgetItem *item = m_tableWidget->item(i, 0);
        if (item) {
            ControlPoint *cp = static_cast<ControlPoint*>(item->data(Qt::UserRole).value<void*>());
            if (cp == point) {
                m_tableWidget->blockSignals(true);
                m_tableWidget->selectRow(i);
                m_tableWidget->blockSignals(false);
                return;
            }
        }
    }
}

void MainWindow::updateCalibData()
{
    m_calibData.clear();
    for (ControlPoint *cp : m_scene->controlPoints()) {
        if (cp->hasId()) {
            m_calibData.addPoint(cp->id(), cp->pos().x(), cp->pos().y());
        }
    }
}

void MainWindow::sortPointsByConfidence()
{
}

void MainWindow::updatePreview(ControlPoint *point)
{
    if (point && m_previewPanel) {
        m_previewPanel->updatePreview(m_imageItem->pixmap(), point->pos());
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_X || event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        ControlPoint *selected = m_scene->selectedPoint();
        if (selected) {
            m_scene->removeControlPoint(selected);
            updateTable();
            updateCalibData();
            if (m_previewPanel) {
                m_previewPanel->updatePreview(m_imageItem->pixmap(), QPointF());
            }
            int count = m_scene->controlPoints().size();
            statusBar()->showMessage(tr("%1 points remaining").arg(count));
            m_clearPointsAction->setEnabled(count > 0);
            m_exportYamlAction->setEnabled(count > 0);
            m_exportCsvAction->setEnabled(count > 0);
        }
        event->accept();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onShowConfigDialog()
{
    if (!m_configDialog) {
        m_configDialog = new QDialog(this);
        m_configDialog->setWindowTitle(tr("Detection Config"));
        QFormLayout *layout = new QFormLayout(m_configDialog);

        QLineEdit *editMargin = new QLineEdit(QString::number(m_detectionConfig.ellipseMargin, 'f', 2), m_configDialog);
        QLineEdit *editMinRatio = new QLineEdit(QString::number(m_detectionConfig.ellipseMinRatio, 'f', 2), m_configDialog);
        QLineEdit *editMaxRatio = new QLineEdit(QString::number(m_detectionConfig.ellipseMaxRatio, 'f', 2), m_configDialog);
        QLineEdit *editCirc = new QLineEdit(QString::number(m_detectionConfig.ellipseMinCircularity, 'f', 2), m_configDialog);
        QLineEdit *editSolidity = new QLineEdit(QString::number(m_detectionConfig.ellipseMinSolidity, 'f', 2), m_configDialog);
        QLineEdit *editUseRefine = new QLineEdit(QString::number(m_detectionConfig.useEllipseRefinement ? 1 : 0), m_configDialog);

        m_configEdits.clear();
        m_configEdits.append(editMargin);
        m_configEdits.append(editMinRatio);
        m_configEdits.append(editMaxRatio);
        m_configEdits.append(editCirc);
        m_configEdits.append(editSolidity);
        m_configEdits.append(editUseRefine);

        layout->addRow("ellipseMargin", editMargin);
        layout->addRow("ellipseMinRatio", editMinRatio);
        layout->addRow("ellipseMaxRatio", editMaxRatio);
        layout->addRow("ellipseMinCircularity", editCirc);
        layout->addRow("ellipseMinSolidity", editSolidity);
        layout->addRow("useEllipseRefinement", editUseRefine);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *okBtn = new QPushButton("OK", m_configDialog);
        QPushButton *cancelBtn = new QPushButton("Cancel", m_configDialog);
        btnLayout->addWidget(okBtn);
        btnLayout->addWidget(cancelBtn);
        layout->addRow(btnLayout);

        connect(okBtn, &QPushButton::clicked, this, [this]() {
            applyDetectionConfig();
            m_configDialog->accept();
        });
        connect(cancelBtn, &QPushButton::clicked, m_configDialog, &QDialog::reject);
    }

    m_configEdits[0]->setText(QString::number(m_detectionConfig.ellipseMargin, 'f', 2));
    m_configEdits[1]->setText(QString::number(m_detectionConfig.ellipseMinRatio, 'f', 2));
    m_configEdits[2]->setText(QString::number(m_detectionConfig.ellipseMaxRatio, 'f', 2));
    m_configEdits[3]->setText(QString::number(m_detectionConfig.ellipseMinCircularity, 'f', 2));
    m_configEdits[4]->setText(QString::number(m_detectionConfig.ellipseMinSolidity, 'f', 2));
    m_configEdits[5]->setText(QString::number(m_detectionConfig.useEllipseRefinement ? 1 : 0));

    m_configDialog->show();
    m_configDialog->raise();
    m_configDialog->activateWindow();
}

void MainWindow::applyDetectionConfig()
{
    if (m_configEdits.size() >= 6) {
        m_detectionConfig.ellipseMargin = m_configEdits[0]->text().toFloat();
        m_detectionConfig.ellipseMinRatio = m_configEdits[1]->text().toFloat();
        m_detectionConfig.ellipseMaxRatio = m_configEdits[2]->text().toFloat();
        m_detectionConfig.ellipseMinCircularity = m_configEdits[3]->text().toFloat();
        m_detectionConfig.ellipseMinSolidity = m_configEdits[4]->text().toFloat();
        m_detectionConfig.useEllipseRefinement = (m_configEdits[5]->text().toInt() != 0);
    }
}
