#include "NVLineChart.h"
#include "../../render/NVRenderer2D.h"
#include "../../data/NVDecimator.h"
#include <cmath>

namespace nv {

NVLineChart::NVLineChart() : NVChart(NV_CHART_LINE) {}

std::vector<NVPoint> NVLineChart::dataToScreen(const NVSeries& series) const {
    if (!m_data) return {};

    auto xView = m_data->getColumn(series.xColumn);
    auto yView = m_data->getColumn(series.yColumn);
    const double* xData = xView.asFloat64();
    const double* yData = yView.asFloat64();
    size_t n = std::min(xView.count, yView.count);

    // Decimate if too many points for the screen width
    std::vector<size_t> indices;
    if (n > static_cast<size_t>(m_plotArea.width * 4)) {
        indices = NVDecimator::autoDecimate(xData, yData, n, m_plotArea.width);
    }

    std::vector<NVPoint> screenPoints;
    if (indices.empty()) {
        screenPoints.reserve(n);
        for (size_t i = 0; i < n; i++) {
            float sx = m_plotArea.x + m_xAxis.dataToPixel(xData[i]);
            float sy = m_plotArea.y + m_yAxis.dataToPixel(yData[i]);
            screenPoints.push_back({sx, sy});
        }
    } else {
        screenPoints.reserve(indices.size());
        for (size_t idx : indices) {
            float sx = m_plotArea.x + m_xAxis.dataToPixel(xData[idx]);
            float sy = m_plotArea.y + m_yAxis.dataToPixel(yData[idx]);
            screenPoints.push_back({sx, sy});
        }
    }

    return screenPoints;
}

std::vector<NVPoint> NVLineChart::smoothPoints(const std::vector<NVPoint>& points) const {
    if (points.size() < 3) return points;

    std::vector<NVPoint> result;
    result.reserve(points.size() * 4); // rough estimate

    float t = m_config.tension;

    for (size_t i = 0; i < points.size() - 1; i++) {
        NVPoint p0 = (i > 0) ? points[i - 1] : points[i];
        NVPoint p1 = points[i];
        NVPoint p2 = points[i + 1];
        NVPoint p3 = (i + 2 < points.size()) ? points[i + 2] : points[i + 1];

        // Catmull-Rom control points
        float dx1 = (p2.x - p0.x) * t;
        float dy1 = (p2.y - p0.y) * t;
        float dx2 = (p3.x - p1.x) * t;
        float dy2 = (p3.y - p1.y) * t;

        // Subdivide into ~8 segments per interval
        int steps = 8;
        for (int s = 0; s <= steps; s++) {
            float u = static_cast<float>(s) / steps;
            float u2 = u * u;
            float u3 = u2 * u;

            // Hermite basis functions
            float h1 = 2 * u3 - 3 * u2 + 1;
            float h2 = u3 - 2 * u2 + u;
            float h3 = -2 * u3 + 3 * u2;
            float h4 = u3 - u2;

            float x = h1 * p1.x + h2 * dx1 + h3 * p2.x + h4 * dx2;
            float y = h1 * p1.y + h2 * dy1 + h3 * p2.y + h4 * dy2;

            result.push_back({x, y});
        }
    }

    return result;
}

void NVLineChart::renderAreaFill(NVRenderer2D& renderer, const std::vector<NVPoint>& screenPoints, NVColor color) const {
    if (screenPoints.size() < 2) return;

    // Fill is a polygon: line points + bottom edge
    float baseline = m_plotArea.y + m_plotArea.height; // Y axis zero line

    NVColor fillColor = color;
    fillColor.a = m_config.fillOpacity;

    // Draw filled triangles from baseline
    for (size_t i = 0; i < screenPoints.size() - 1; i++) {
        NVPoint a = screenPoints[i];
        NVPoint b = screenPoints[i + 1];

        // Two triangles forming a quad: a, b, b_bottom, a_bottom
        // For now, use thin rectangles (SDF renderer doesn't do arbitrary tris yet)
        float x = std::min(a.x, b.x);
        float w = std::abs(b.x - a.x);
        float y = std::min(a.y, std::min(b.y, baseline));
        float h = baseline - y;

        if (w > 0 && h > 0) {
            renderer.drawRect({x, y, w, h}, fillColor);
        }
    }
}

void NVLineChart::renderChart(NVRenderer2D& renderer) {
    for (size_t si = 0; si < m_series.size(); si++) {
        const auto& series = m_series[si];
        if (!series.visible) continue;

        auto screenPoints = dataToScreen(series);
        if (screenPoints.size() < 2) continue;

        // Smooth if configured
        if (m_config.smoothCurve) {
            screenPoints = smoothPoints(screenPoints);
        }

        // Area fill
        if (m_config.fillArea) {
            renderAreaFill(renderer, screenPoints, series.color);
        }

        // Line
        LineParams lp;
        lp.color = series.color;
        lp.width = series.lineWidth;
        lp.join = NV_LINE_JOIN_ROUND;
        lp.cap = NV_LINE_CAP_ROUND;
        renderer.drawPolyline(screenPoints.data(), screenPoints.size(), lp);

        // Markers
        if (m_config.showMarkers && series.marker != NV_MARKER_NONE) {
            // Don't draw markers if too many points (performance)
            size_t maxMarkers = static_cast<size_t>(m_plotArea.width / (series.markerSize * 3));
            size_t step = std::max<size_t>(1, screenPoints.size() / maxMarkers);

            for (size_t i = 0; i < screenPoints.size(); i += step) {
                NVPoint p = screenPoints[i];
                float r = series.markerSize;

                switch (series.marker) {
                    case NV_MARKER_CIRCLE:
                        renderer.drawCircle(p.x, p.y, r, series.color);
                        break;
                    case NV_MARKER_SQUARE:
                        renderer.drawRect({p.x - r, p.y - r, r * 2, r * 2}, series.color);
                        break;
                    case NV_MARKER_DIAMOND:
                        renderer.drawPolygon(p.x, p.y, r, 4, series.color);
                        break;
                    case NV_MARKER_TRIANGLE:
                        renderer.drawPolygon(p.x, p.y, r, 3, series.color);
                        break;
                    case NV_MARKER_CROSS: {
                        LineParams mp;
                        mp.color = series.color;
                        mp.width = 2;
                        renderer.drawLine({p.x - r, p.y}, {p.x + r, p.y}, mp);
                        renderer.drawLine({p.x, p.y - r}, {p.x, p.y + r}, mp);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

NVChart::HitResult NVLineChart::hitTest(NVPoint point) const {
    HitResult result;
    float minDist = 20.0f; // max hit distance in pixels

    for (size_t si = 0; si < m_series.size(); si++) {
        if (!m_series[si].visible || !m_data) continue;

        auto xView = m_data->getColumn(m_series[si].xColumn);
        auto yView = m_data->getColumn(m_series[si].yColumn);
        const double* xData = xView.asFloat64();
        const double* yData = yView.asFloat64();
        size_t n = std::min(xView.count, yView.count);

        for (size_t i = 0; i < n; i++) {
            float sx = m_plotArea.x + m_xAxis.dataToPixel(xData[i]);
            float sy = m_plotArea.y + m_yAxis.dataToPixel(yData[i]);

            float dx = point.x - sx;
            float dy = point.y - sy;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < minDist) {
                minDist = dist;
                result.seriesIndex = static_cast<int>(si);
                result.pointIndex = i;
                result.xValue = xData[i];
                result.yValue = yData[i];
                result.screenPos = {sx, sy};
            }
        }
    }

    return result;
}

} // namespace nv
