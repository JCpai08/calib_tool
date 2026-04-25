#include <QApplication>
#include "app/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Camera Calibration Tool");
    app.setOrganizationName("CalibTool");

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}