#ifndef CIRCLEDETECTOR_H
#define CIRCLEDETECTOR_H

#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

struct CirclePoint
{
    float x;
    float y;
    float radius;
    float area;
    float circularity;
    float confidence;
    std::string source;
};

struct CircleDetectionConfig
{
    float minAreaRatio = 1e-6f;
    float maxAreaRatio = 3e-3f;
    float minCircularity = 0.72f;
    float minRadiusPx = 3.0f;
    float maxRadiusPx = 80.0f;
    float mergeDistancePx = 8.0f;
    bool useHough = false;
    float houghDp = 1.2f;
    float houghMinDist = 10.0f;
    float houghParam1 = 120.0f;
    float houghParam2 = 18.0f;
    float maxInnerIntensity = 170.0f;
    float minRingContrast = 18.0f;

    float ellipseMargin = 5.0f;
    float ellipseMinRatio = 0.7f;
    float ellipseMaxRatio = 1.4f;
    float ellipseMinCircularity = 0.7f;
    float ellipseMinSolidity = 0.8f;
    int ellipseMinContourPoints = 5;
    // bool useEllipseRefinement = false;
    bool useEllipseRefinement = true;

    bool debug = false;
};

class CircleDetector
{
public:
    explicit CircleDetector(const CircleDetectionConfig &config = CircleDetectionConfig{});

    void setConfig(const CircleDetectionConfig &config);

    std::vector<CirclePoint> detect(const cv::Mat &image);

    std::vector<CirclePoint> detectInRoi(const cv::Mat &image, const cv::Rect &roi);

    cv::Mat getDebugGray() const { return m_debugGray; }
    cv::Mat getDebugBinary() const { return m_debugBinary; }

    const CircleDetectionConfig& getConfig() const { return m_config; }

private:
    CircleDetectionConfig m_config;

    cv::Mat binaryForDarkMarkers(const cv::Mat &gray);

    std::vector<CirclePoint> fromContours(const cv::Mat &binary);

    std::vector<CirclePoint> fromHough(const cv::Mat &gray);

    std::vector<CirclePoint> mergeCloseMarkers(const std::vector<CirclePoint> &markers);

    bool passesIntensityFilter(const cv::Mat &gray, const CirclePoint &marker);

    bool refineWithEllipseFitting(const cv::Mat &image, CirclePoint &marker);

    mutable cv::Mat m_debugGray;
    mutable cv::Mat m_debugBinary;
    int m_idCounterGlobal = 0;
};

#endif // CIRCLEDETECTOR_H