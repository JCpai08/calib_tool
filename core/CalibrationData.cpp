#include "CalibrationData.h"
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QDebug>
#include <algorithm>
#include <utility>

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

bool CalibrationData::importYaml(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return false;
    }

    std::vector<CalibrationPoint> importedPoints;
    double importedWidth = 0;
    double importedHeight = 0;

    const QRegularExpression imageWidthRe(R"(^\s*image_width\s*:\s*([-+]?\d*\.?\d+)\s*$)");
    const QRegularExpression imageHeightRe(R"(^\s*image_height\s*:\s*([-+]?\d*\.?\d+)\s*$)");
    const QRegularExpression pointRe(
        R"(^\s*-\s*\{\s*id\s*:\s*(-?\d+)\s*,\s*x\s*:\s*([-+]?\d*\.?\d+)\s*,\s*y\s*:\s*([-+]?\d*\.?\d+)\s*\}\s*$)");

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();

        QRegularExpressionMatch match = imageWidthRe.match(line);
        if (match.hasMatch()) {
            importedWidth = match.captured(1).toDouble();
            continue;
        }

        match = imageHeightRe.match(line);
        if (match.hasMatch()) {
            importedHeight = match.captured(1).toDouble();
            continue;
        }

        match = pointRe.match(line);
        if (match.hasMatch()) {
            importedPoints.push_back({
                match.captured(1).toInt(),
                match.captured(2).toDouble(),
                match.captured(3).toDouble()
            });
        }
    }

    if (importedPoints.empty()) {
        qWarning() << "No calibration points found in YAML:" << filePath;
        return false;
    }

    m_points = std::move(importedPoints);
    m_imageWidth = importedWidth;
    m_imageHeight = importedHeight;
    return true;
}

bool CalibrationData::importCsv(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << filePath;
        return false;
    }

    std::vector<CalibrationPoint> importedPoints;
    QTextStream in(&file);
    bool firstDataLine = true;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList fields = line.split(',');
        if (fields.size() < 3) {
            qWarning() << "Invalid CSV line:" << line;
            return false;
        }

        if (firstDataLine && fields[0].trimmed().compare("id", Qt::CaseInsensitive) == 0) {
            firstDataLine = false;
            continue;
        }
        firstDataLine = false;

        bool idOk = false;
        bool xOk = false;
        bool yOk = false;
        const int id = fields[0].trimmed().toInt(&idOk);
        const double x = fields[1].trimmed().toDouble(&xOk);
        const double y = fields[2].trimmed().toDouble(&yOk);

        if (!idOk || !xOk || !yOk) {
            qWarning() << "Invalid CSV values:" << line;
            return false;
        }

        importedPoints.push_back({id, x, y});
    }

    if (importedPoints.empty()) {
        qWarning() << "No calibration points found in CSV:" << filePath;
        return false;
    }

    m_points = std::move(importedPoints);
    m_imageWidth = 0;
    m_imageHeight = 0;
    return true;
}
