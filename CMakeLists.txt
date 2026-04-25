cmake_minimum_required(VERSION 3.16)
project(calib_tool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED)

add_executable(calib_tool
    main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    graphics/ZoomableView.cpp
    graphics/ZoomableView.h
    graphics/ControlPoint.cpp
    graphics/ControlPoint.h
    graphics/ImageItem.cpp
    graphics/ImageItem.h
    graphics/MagnifierPopup.cpp
    graphics/MagnifierPopup.h
    graphics/ControlPointScene.cpp
    graphics/ControlPointScene.h
    core/CircleDetector.cpp
    core/CircleDetector.h
    core/CalibrationData.cpp
    core/CalibrationData.h
)

target_link_libraries(calib_tool Qt6::Widgets ${OpenCV_LIBS})