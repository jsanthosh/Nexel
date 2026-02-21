#include "NVScatterChart.h"
#include "../../render/NVRenderer2D.h"
#include <cmath>
#include <algorithm>

namespace nv {

NVScatterChart::NVScatterChart() : NVChart(NV_CHART_SCATTER) {}

void NVScatterChart::renderChart(NVRenderer2D& renderer) {
    if (!m_data) return;

    // Size mapping range
    double sizeMin = 0, sizeMax = 1;
    if (m_config.sizeMapping && m_config.sizeColumn >= 0) {
        sizeMin = m_data->minValue(m_config.sizeColumn);
        sizeMax = m_data->maxValue(m_config.sizeColumn);
    }

    for (size_t si = 0; si < m_series.size(); si++) {
        const auto& series = m_series[si];
        if (!series.visible) continue;

        auto xView = m_data->getColumn(series.xColumn);
        auto yView = m_data->getColumn(series.yColumn);
        const double* xData = xView.asFloat64();
        const double* yData = yView.asFloat64();
        size_t n = std::min(xView.count, yView.count);

        const double* sizeData = nullptr;
        if (m_config.sizeMapping && m_config.sizeColumn >= 0) {
            sizeData = m_data->getColumn(m_config.sizeColumn).asFloat64();
        }

        NVColor color = series.color;
        color.a = m_config.markerOpacity;

        for (size_t i = 0; i < n; i++) {
            float sx = m_plotArea.x + m_xAxis.dataToPixel(xData[i]);
            float sy = m_plotArea.y + m_yAxis.dataToPixel(yData[i]);

            // Skip if outside plot area
            if (sx < m_plotArea.x || sx > m_plotArea.x + m_plotArea.width ||
                sy < m_plotArea.y || sy > m_plotArea.y + m_plotArea.height) {
                continue;
            }

            float size = m_config.markerSize;
            if (sizeData) {
                double sizeRange = sizeMax - sizeMin;
                if (sizeRange > 0) {
                    float t = static_cast<float>((sizeData[i] - sizeMin) / sizeRange);
                    size = m_config.minMarkerSize + t * (m_config.maxMarkerSize - m_config.minMarkerSize);
                }
            }

            switch (m_config.markerShape) {
                case NV_MARKER_CIRCLE:
                    renderer.drawCircle(sx, sy, size, color);
                    break;
                case NV_MARKER_SQUARE:
                    renderer.drawRect({sx - size, sy - size, size * 2, size * 2}, color);
                    break;
                case NV_MARKER_DIAMOND:
                    renderer.drawPolygon(sx, sy, size, 4, color);
                    break;
                case NV_MARKER_TRIANGLE:
                    renderer.drawPolygon(sx, sy, size, 3, color);
                    break;
                default:
                    renderer.drawCircle(sx, sy, size, color);
                    break;
            }
        }

        // Trend line
        if (m_config.showTrendLine) {
            renderTrendLine(renderer, series);
        }
    }
}

void NVScatterChart::renderTrendLine(NVRenderer2D& renderer, const NVSeries& series) const {
    auto xView = m_data->getColumn(series.xColumn);
    auto yView = m_data->getColumn(series.yColumn);
    const double* xData = xView.asFloat64();
    const double* yData = yView.asFloat64();
    size_t n = std::min(xView.count, yView.count);
    if (n < 2) return;

    // Simple linear regression: y = mx + b
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (size_t i = 0; i < n; i++) {
        sumX += xData[i];
        sumY += yData[i];
        sumXY += xData[i] * yData[i];
        sumXX += xData[i] * xData[i];
    }
    double nD = static_cast<double>(n);
    double denom = nD * sumXX - sumX * sumX;
    if (std::abs(denom) < 1e-10) return;

    double m = (nD * sumXY - sumX * sumY) / denom;
    double b = (sumY - m * sumX) / nD;

    double xMin = m_xAxis.effectiveMin();
    double xMax = m_xAxis.effectiveMax();
    double y1 = m * xMin + b;
    double y2 = m * xMax + b;

    NVPoint p1 = {m_plotArea.x + m_xAxis.dataToPixel(xMin),
                  m_plotArea.y + m_yAxis.dataToPixel(y1)};
    NVPoint p2 = {m_plotArea.x + m_xAxis.dataToPixel(xMax),
                  m_plotArea.y + m_yAxis.dataToPixel(y2)};

    LineParams lp;
    lp.color = series.color;
    lp.color.a = 0.5f;
    lp.width = 1.5f;
    lp.dashLength = 8;
    lp.gapLength = 4;
    renderer.drawLine(p1, p2, lp);
}

NVChart::HitResult NVScatterChart::hitTest(NVPoint point) const {
    HitResult result;
    float minDist = m_config.markerSize * 2 + 5;

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
