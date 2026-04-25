#ifndef CALIBRATIONDATA_H
#define CALIBRATIONDATA_H

#include <vector>
#include <QPointF>
#include <QString>

struct CalibrationPoint
{
    int id;
    double x, y;
};

class CalibrationData
{
public:
    CalibrationData() = default;

    void addPoint(int id, double x, double y);
    void removePoint(int id);
    void clear();

    const std::vector<CalibrationPoint> &points() const { return m_points; }
    std::vector<CalibrationPoint> &points() { return m_points; }

    bool exportYaml(const QString &filePath) const;
    bool exportCsv(const QString &filePath) const;

    int pointCount() const { return static_cast<int>(m_points.size()); }

    double imageWidth() const { return m_imageWidth; }
    double imageHeight() const { return m_imageHeight; }
    void setImageSize(double w, double h) { m_imageWidth = w; m_imageHeight = h; }

private:
    std::vector<CalibrationPoint> m_points;
    double m_imageWidth = 0;
    double m_imageHeight = 0;
};

#endif // CALIBRATIONDATA_H