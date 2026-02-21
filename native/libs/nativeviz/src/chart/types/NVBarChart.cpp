#include "NVBarChart.h"
#include "../../render/NVRenderer2D.h"
#include <cmath>
#include <algorithm>

namespace nv {

NVBarChart::NVBarChart() : NVChart(NV_CHART_BAR) {
    m_xAxisConfig.type = NV_AXIS_CATEGORY;
}

void NVBarChart::computeDataRange(double& xMin, double& xMax, double& yMin, double& yMax) {
    if (!m_data || m_series.empty()) {
        xMin = 0; xMax = 1; yMin = 0; yMax = 1;
        return;
    }

    size_t n = m_data->rowCount();
    xMin = -0.5;
    xMax = static_cast<double>(n) - 0.5;

    yMin = 0; // bars always start at 0
    yMax = 0;

    if (m_config.mode == NVBarMode::Stacked || m_config.mode == NVBarMode::Stacked100) {
        // Sum per category
        for (size_t i = 0; i < n; i++) {
            double sum = 0;
            for (const auto& s : m_series) {
                if (!s.visible) continue;
                auto col = m_data->getColumn(s.yColumn);
                sum += col.asFloat64()[i];
            }
            yMax = std::max(yMax, sum);
        }
        if (m_config.mode == NVBarMode::Stacked100) {
            yMax = 100;
        }
    } else {
        for (const auto& s : m_series) {
            if (!s.visible) continue;
            double sMax = m_data->maxValue(s.yColumn);
            double sMin = m_data->minValue(s.yColumn);
            yMax = std::max(yMax, sMax);
            yMin = std::min(yMin, sMin);
        }
    }

    if (yMin > 0) yMin = 0;
}

std::vector<NVBarChart::BarRect> NVBarChart::computeBarRects() const {
    std::vector<BarRect> bars;
    if (!m_data || m_series.empty()) return bars;

    size_t n = m_data->rowCount();
    size_t seriesCount = 0;
    for (const auto& s : m_series) if (s.visible) seriesCount++;
    if (seriesCount == 0) return bars;

    float categoryWidth = m_plotArea.width / n;
    float barGroupWidth = categoryWidth * m_config.barWidthFraction;
    float barWidth = barGroupWidth / (m_config.mode == NVBarMode::Grouped ? seriesCount : 1);
    float baseline = m_plotArea.y + m_yAxis.dataToPixel(0);

    for (size_t i = 0; i < n; i++) {
        float categoryCenter = m_plotArea.x + m_xAxis.dataToPixel(static_cast<double>(i));
        float groupStart = categoryCenter - barGroupWidth * 0.5f;
        double stackY = 0;

        // For stacked100, compute total
        double total100 = 0;
        if (m_config.mode == NVBarMode::Stacked100) {
            for (const auto& s : m_series) {
                if (!s.visible) continue;
                total100 += m_data->getColumn(s.yColumn).asFloat64()[i];
            }
        }

        int visIdx = 0;
        for (size_t si = 0; si < m_series.size(); si++) {
            const auto& s = m_series[si];
            if (!s.visible) continue;

            double value = m_data->getColumn(s.yColumn).asFloat64()[i];

            if (m_config.mode == NVBarMode::Stacked100 && total100 > 0) {
                value = (value / total100) * 100.0;
            }

            BarRect bar;
            bar.seriesIndex = static_cast<int>(si);
            bar.pointIndex = i;
            bar.color = s.color;

            if (m_config.mode == NVBarMode::Grouped) {
                float barX = groupStart + visIdx * barWidth;
                float barTop = m_plotArea.y + m_yAxis.dataToPixel(value);
                float barH = baseline - barTop;

                if (barH < 0) { barTop = baseline; barH = -barH; }

                bar.rect = {barX, barTop, barWidth - 1, barH};
            } else {
                // Stacked
                float barX = categoryCenter - barWidth * 0.5f;
                float stackTop = m_plotArea.y + m_yAxis.dataToPixel(stackY + value);
                float stackBottom = m_plotArea.y + m_yAxis.dataToPixel(stackY);
                float barH = stackBottom - stackTop;

                bar.rect = {barX, stackTop, barWidth - 1, barH};
                stackY += value;
            }

            bars.push_back(bar);
            visIdx++;
        }
    }

    return bars;
}

void NVBarChart::renderChart(NVRenderer2D& renderer) {
    auto bars = computeBarRects();

    for (const auto& bar : bars) {
        renderer.drawRect(bar.rect, bar.color, m_config.cornerRadius);

        // Value labels
        if (m_config.showLabels) {
            const auto& s = m_series[bar.seriesIndex];
            auto yView = m_data->getColumn(s.yColumn);
            double value = yView.asFloat64()[bar.pointIndex];

            char label[32];
            snprintf(label, sizeof(label), "%.1f", value);

            float labelX = bar.rect.x + bar.rect.width * 0.5f;
            float labelY = bar.rect.y - 4;
            renderer.drawText(label, {labelX, labelY}, m_theme.axisLabelColor, m_theme.tickFontSize);
        }
    }
}

NVChart::HitResult NVBarChart::hitTest(NVPoint point) const {
    auto bars = computeBarRects();
    for (const auto& bar : bars) {
        if (point.x >= bar.rect.x && point.x <= bar.rect.x + bar.rect.width &&
            point.y >= bar.rect.y && point.y <= bar.rect.y + bar.rect.height) {

            HitResult result;
            result.seriesIndex = bar.seriesIndex;
            result.pointIndex = bar.pointIndex;

            const auto& s = m_series[bar.seriesIndex];
            auto xView = m_data->getColumn(s.xColumn);
            auto yView = m_data->getColumn(s.yColumn);
            result.xValue = xView.asFloat64()[bar.pointIndex];
            result.yValue = yView.asFloat64()[bar.pointIndex];
            result.screenPos = {bar.rect.x + bar.rect.width * 0.5f, bar.rect.y};
            return result;
        }
    }
    return {};
}

} // namespace nv
