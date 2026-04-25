#include "CalibrationData.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

void CalibrationData::addPoint(int id, double x, double y)
{
    m_points.push_back({id, x, y});
}

void CalibrationData::removePoint(int id)
{
    m_points.erase(
        std::remove_if(m_points.begin(), m_points.end(),
                      [id](const CalibrationPoint &p) { return p.id == id; }),
        m_points.end());
}

void CalibrationData::clear()
{
    m_points.clear();
}

bool CalibrationData::exportYaml(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out << "# Calibration Points Export\n";
    out << "# Image size:" << m_imageWidth << "x" << m_imageHeight << "\n\n";
    out << "image_width: " << m_imageWidth << "\n";
    out << "image_height: " << m_imageHeight << "\n";
    out << "points:\n";

    for (const auto &p : m_points) {
        out << "  - {id: " << p.id << ", x: " << QString::number(p.x, 'f', 3)
            << ", y: " << QString::number(p.y, 'f', 3) << "}\n";
    }

    file.close();
    return true;
}

bool CalibrationData::exportCsv(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out << "id,x,y\n";

    for (const auto &p : m_points) {
        out << p.id << "," << QString::number(p.x, 'f', 3)
            << "," << QString::number(p.y, 'f', 3) << "\n";
    }

    file.close();
    return true;
}