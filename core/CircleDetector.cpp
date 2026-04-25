#include "CircleDetector.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

const float PI = 3.14159265358979323846f;

static float calcMarkerConfidence(float circularity, float solidity, float axisRatio);
static std::vector<CirclePoint> calcConfidence(std::vector<CirclePoint> &markers);

CircleDetector::CircleDetector(const CircleDetectionConfig &config)
    : m_config(config)
{
}

void CircleDetector::setConfig(const CircleDetectionConfig &config)
{
    m_config = config;
}

cv::Mat CircleDetector::binaryForDarkMarkers(const cv::Mat &gray)
{
    cv::Mat grayInput;
    if (gray.channels() == 3) {
        cv::cvtColor(gray, grayInput, cv::COLOR_BGR2GRAY);
    } else {
        grayInput = gray;
    }

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    cv::Mat normalized;
    clahe->apply(grayInput, normalized);

    cv::GaussianBlur(normalized, normalized, cv::Size(5, 5), 0);

    cv::Mat binary;
    cv::adaptiveThreshold(
        normalized,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        31,
        6
    );

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);

    m_debugGray = normalized.clone();
    m_debugBinary = binary.clone();

    return binary;
}

std::vector<CirclePoint> CircleDetector::fromContours(const cv::Mat &binary)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    if (m_config.debug) {
        printf("Found %zu contours in binary image\n", contours.size());
    }

    float imageArea = static_cast<float>(binary.rows * binary.cols);
    float minArea = imageArea * m_config.minAreaRatio;
    float maxArea = imageArea * m_config.maxAreaRatio;

    std::vector<CirclePoint> markers;
    for (const auto &contour : contours) {
        float area = static_cast<float>(cv::contourArea(contour));
        if (area < minArea || area > maxArea) {
            continue;
        }

        float perimeter = static_cast<float>(cv::arcLength(contour, true));
        if (perimeter <= 0) {
            continue;
        }

        float circularity = 4.0f * PI * area / (perimeter * perimeter);
        if (circularity < m_config.minCircularity) {
            continue;
        }

        cv::Point2f center;
        float radius;
        cv::minEnclosingCircle(contour, center, radius);
        if (radius < m_config.minRadiusPx || radius > m_config.maxRadiusPx) {
            continue;
        }

        CirclePoint marker;
        marker.x = center.x;
        marker.y = center.y;
        marker.radius = radius;
        marker.area = area;
        marker.circularity = circularity;
        marker.confidence = 0.5f;
        marker.source = "contour";
        markers.push_back(marker);
    }

    if (markers.empty()) {
        return markers;
    }

    float meanRadius = 0.0f;
    for (const auto &m : markers) {
        meanRadius += m.radius;
    }
    meanRadius /= markers.size();

    std::vector<CirclePoint> filtered;
    for (const auto &m : markers) {
        if (m.radius >= 0.5f * meanRadius) {
            filtered.push_back(m);
        }
    }

    return calcConfidence(filtered);
}

std::vector<CirclePoint> CircleDetector::fromHough(const cv::Mat &gray)
{
    cv::Mat grayInput;
    if (gray.channels() == 3) {
        cv::cvtColor(gray, grayInput, cv::COLOR_BGR2GRAY);
    } else {
        grayInput = gray;
    }

    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        grayInput,
        circles,
        cv::HOUGH_GRADIENT,
        m_config.houghDp,
        m_config.houghMinDist,
        m_config.houghParam1,
        m_config.houghParam2,
        static_cast<int>(m_config.minRadiusPx),
        static_cast<int>(m_config.maxRadiusPx)
    );

    std::vector<CirclePoint> markers;
    for (const auto &c : circles) {
        float x = c[0];
        float y = c[1];
        float radius = c[2];
        float area = PI * radius * radius;

        CirclePoint marker;
        marker.x = x;
        marker.y = y;
        marker.radius = radius;
        marker.area = area;
        marker.circularity = 1.0f;
        marker.confidence = 0.7f;
        marker.source = "hough";
        markers.push_back(marker);
    }

    return markers;
}

std::vector<CirclePoint> CircleDetector::mergeCloseMarkers(const std::vector<CirclePoint> &markers)
{
    if (markers.empty()) {
        return markers;
    }

    std::vector<CirclePoint> sortedMarkers = markers;
    std::sort(sortedMarkers.begin(), sortedMarkers.end(),
              [](const CirclePoint &a, const CirclePoint &b) {
                  return a.radius > b.radius;
              });

    std::vector<CirclePoint> merged;
    for (const auto &marker : sortedMarkers) {
        bool keep = true;
        for (const auto &existing : merged) {
            float dx = marker.x - existing.x;
            float dy = marker.y - existing.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= m_config.mergeDistancePx) {
                keep = false;
                break;
            }
        }
        if (keep) {
            merged.push_back(marker);
        }
    }

    std::sort(merged.begin(), merged.end(),
              [](const CirclePoint &a, const CirclePoint &b) {
                  if (std::abs(a.y - b.y) > 10.0f) {
                      return a.y < b.y;
                  }
                  return a.x < b.x;
              });

    for (auto &m : merged) {
        m.source = "merged";
    }

    return merged;
}

bool CircleDetector::passesIntensityFilter(const cv::Mat &gray, const CirclePoint &marker)
{
    cv::Mat grayInput;
    if (gray.channels() == 3) {
        cv::cvtColor(gray, grayInput, cv::COLOR_BGR2GRAY);
    } else {
        grayInput = gray;
    }

    int radius = std::max(2, static_cast<int>(std::round(marker.radius)));
    int cx = static_cast<int>(std::round(marker.x));
    int cy = static_cast<int>(std::round(marker.y));

    int y0 = std::max(0, cy - 2 * radius);
    int y1 = std::min(grayInput.rows, cy + 2 * radius + 1);
    int x0 = std::max(0, cx - 2 * radius);
    int x1 = std::min(grayInput.cols, cx + 2 * radius + 1);

    cv::Mat roi = grayInput(cv::Rect(x0, y0, x1 - x0, y1 - y0));
    if (roi.empty()) {
        return false;
    }

    int roiCx = cx - x0;
    int roiCy = cy - y0;

    cv::Mat distMask(roi.rows, roi.cols, CV_32FC1);
    for (int y = 0; y < roi.rows; ++y) {
        for (int x = 0; x < roi.cols; ++x) {
            float dx = static_cast<float>(x - roiCx);
            float dy = static_cast<float>(y - roiCy);
            distMask.at<float>(y, x) = dx * dx + dy * dy;
        }
    }

    float innerRadiusSq = static_cast<float>((0.75 * radius) * (0.75 * radius));
    float ringInnerSq = static_cast<float>((1.1 * radius) * (1.1 * radius));
    float ringOuterSq = static_cast<float>((1.8 * radius) * (1.8 * radius));

    int innerCount = 0;
    int ringCount = 0;
    float innerSum = 0.0f;
    float ringSum = 0.0f;

    for (int y = 0; y < roi.rows; ++y) {
        for (int x = 0; x < roi.cols; ++x) {
            float dist2 = distMask.at<float>(y, x);
            if (dist2 <= innerRadiusSq) {
                innerCount++;
                innerSum += static_cast<float>(roi.at<uchar>(y, x));
            } else if (dist2 >= ringInnerSq && dist2 <= ringOuterSq) {
                ringCount++;
                ringSum += static_cast<float>(roi.at<uchar>(y, x));
            }
        }
    }

    if (innerCount < 12 || ringCount < 20) {
        return false;
    }

    float innerMean = innerSum / innerCount;
    float ringMean = ringSum / ringCount;

    return innerMean <= m_config.maxInnerIntensity &&
           (ringMean - innerMean) >= m_config.minRingContrast;
}

std::vector<CirclePoint> CircleDetector::detect(const cv::Mat &image)
{
    if (image.empty()) {
        return {};
    }

    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }

    cv::Mat binary = binaryForDarkMarkers(gray);

    std::vector<CirclePoint> markers = fromContours(binary);

    if (m_config.useHough) {
        std::vector<CirclePoint> houghMarkers = fromHough(gray);
        markers.insert(markers.end(), houghMarkers.begin(), houghMarkers.end());
    }

    std::vector<CirclePoint> filtered;
    for (const auto &marker : markers) {
        if (passesIntensityFilter(gray, marker)) {
            filtered.push_back(marker);
        }
    }

    std::vector<CirclePoint> merged = mergeCloseMarkers(filtered);

    if (m_config.useEllipseRefinement) {
        std::vector<CirclePoint> refined;
        for (auto &marker : merged) {
            if (refineWithEllipseFitting(image, marker)) {
                refined.push_back(marker);
            }
        }
        merged = std::move(refined);
    }

    for (auto &m : merged) {
        m.source = "contour";
        m_idCounterGlobal++;
    }

    return merged;
}

std::vector<CirclePoint> CircleDetector::detectInRoi(const cv::Mat &image, const cv::Rect &roi)
{
    if (roi.width <= 0 || roi.height <= 0 || image.empty()) {
        return detect(image);
    }

    int h = image.rows;
    int w = image.cols;

    int x0 = std::max(0, std::min(roi.x, w - 1));
    int y0 = std::max(0, std::min(roi.y, h - 1));
    int x1 = std::max(x0 + 1, std::min(roi.x + roi.width, w));
    int y1 = std::max(y0 + 1, std::min(roi.y + roi.height, h));

    cv::Rect validRoi(x0, y0, x1 - x0, y1 - y0);
    cv::Mat roiImage = image(validRoi);

    std::vector<CirclePoint> localMarkers = detect(roiImage);

    std::vector<CirclePoint> globalMarkers;
    for (const auto &marker : localMarkers) {
        CirclePoint globalMarker = marker;
        globalMarker.x += static_cast<float>(x0);
        globalMarker.y += static_cast<float>(y0);
        globalMarker.source = "roi_" + marker.source;
        globalMarkers.push_back(globalMarker);
    }

    return globalMarkers;
}

static float computeCircularity(float area, float perimeter)
{
    if (perimeter <= 0) {
        return 0.0f;
    }
    return 4.0f * PI * area / (perimeter * perimeter);
}

static float computeSolidity(float contourArea, float majorAxis, float minorAxis)
{
    float ellipseArea = PI * majorAxis * minorAxis / 4.0f;
    if (ellipseArea <= 0) {
        return 0.0f;
    }
    return contourArea / ellipseArea;
}

bool CircleDetector::refineWithEllipseFitting(const cv::Mat &image, CirclePoint &marker)
{
    cv::Mat grayInput;
    if (image.channels() == 3) {
        cv::cvtColor(image, grayInput, cv::COLOR_BGR2GRAY);
    } else {
        grayInput = image;
    }

    int cx = static_cast<int>(std::round(marker.x));
    int cy = static_cast<int>(std::round(marker.y));
    int radius = static_cast<int>(std::round(marker.radius));
    int margin = static_cast<int>(std::round(m_config.ellipseMargin));

    int y1 = std::max(0, cy - radius - margin);
    int y2 = std::min(image.rows, cy + radius + margin);
    int x1 = std::max(0, cx - radius - margin);
    int x2 = std::min(image.cols, cx + radius + margin);

    if (y2 <= y1 || x2 <= x1) {
        return false;
    }

    cv::Mat crop = grayInput(cv::Rect(x1, y1, x2 - x1, y2 - y1));
    if (crop.empty()) {
        return false;
    }

    cv::Mat binary;
    cv::threshold(crop, binary, 1, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    if (contours.empty()) {
        return false;
    }

    auto targetIt = std::max_element(contours.begin(), contours.end(),
                                [](const std::vector<cv::Point> &a, const std::vector<cv::Point> &b) {
                                    return cv::contourArea(a) < cv::contourArea(b);
                                });
    const auto &target = *targetIt;

    if (static_cast<int>(target.size()) < m_config.ellipseMinContourPoints) {
        return false;
    }

    cv::RotatedRect ellipse = cv::fitEllipse(target);
    cv::Point2f ellipseCenter = ellipse.center;
    float majorAxis = ellipse.size.width;
    float minorAxis = ellipse.size.height;
    float angle = ellipse.angle;

    if (minorAxis > majorAxis) {
        std::swap(majorAxis, minorAxis);
    }

    float ratio = majorAxis / minorAxis;

    float contourArea = static_cast<float>(cv::contourArea(target));
    float perimeter = static_cast<float>(cv::arcLength(target, true));
    float circularity = computeCircularity(contourArea, perimeter);
    float solidity = computeSolidity(contourArea, majorAxis, minorAxis);

    bool isValid = (ratio > m_config.ellipseMinRatio) &&
                   (ratio < m_config.ellipseMaxRatio) &&
                   (circularity > m_config.ellipseMinCircularity) &&
                   (solidity > m_config.ellipseMinSolidity);

    if (!isValid) {
        return false;
    }

    marker.x = static_cast<float>(ellipseCenter.x) + static_cast<float>(x1);
    marker.y = static_cast<float>(ellipseCenter.y) + static_cast<float>(y1);
    marker.source = "ellipse_refined";

    float circ = computeCircularity(contourArea, perimeter);
    float solidityVal = computeSolidity(contourArea, majorAxis, minorAxis);
    marker.confidence = calcMarkerConfidence(circ, solidityVal, ratio);

    return true;
}

static float calcMarkerConfidence(float circularity, float solidity, float axisRatio)
{
    float ratioScore = 1.0f - std::abs(1.0f - axisRatio);
    ratioScore = std::max(0.0f, ratioScore);

    float score = circularity * 0.4f + solidity * 0.4f + ratioScore * 0.2f;
    return std::max(0.0f, std::min(1.0f, score));
}

std::vector<CirclePoint> calcConfidence(std::vector<CirclePoint> &markers)
{
    if (markers.empty()) {
        return markers;
    }

    float meanCirc = 0.0f;
    for (const auto &m : markers) {
        meanCirc += m.circularity;
    }
    meanCirc /= markers.size();

    float sumVar = 0.0f;
    for (const auto &m : markers) {
        float diff = m.circularity - meanCirc;
        sumVar += diff * diff;
    }
    float stdCirc = std::sqrt(sumVar / markers.size());

    for (auto &m : markers) {
        float circScore = (stdCirc > 0.0f) ? ((m.circularity - meanCirc) / (3.0f * stdCirc) + 1.0f) : 1.0f;
        circScore = std::max(0.0f, std::min(1.0f, circScore));

        float areaScore = (m.circularity > 0.8f) ? 1.0f : (m.circularity / 0.8f);
        areaScore = std::max(0.0f, std::min(1.0f, areaScore));

        m.confidence = circScore * 0.6f + areaScore * 0.4f;
    }

    return markers;
}