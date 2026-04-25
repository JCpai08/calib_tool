# AGENTS.md

## Build

```bash
cd /home/jcpai08/project/calib_tool
mkdir -p build && cd build
cmake ..
make
./calib_tool
```

## Project Structure

- `app/MainWindow.{cpp,h}` - Main window with menu/toolbar
- `graphics/` - Qt graphics items (ZoomableView, ControlPointScene, ImageItem, MagnifierPopup)
- `core/` - Business logic (CircleDetector uses OpenCV, CalibrationData for point management)
- `main.cpp` - Entry point, creates QApplication and MainWindow

## Dependencies

- Qt6 (Widgets)
- OpenCV (for circle detection via SimpleBlobDetector)

## Key Conventions

- C++17 standard
- Qt6 `Q_OBJECT` macros require moc (handled by CMake AUTOMOC)
- Point double-click clears, single-click selects
- Export formats: YAML (.yml), CSV

## No Tests

This repo has no test suite.