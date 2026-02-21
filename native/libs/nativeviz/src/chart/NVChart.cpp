#include "NVChart.h"
#include "../render/NVRenderer2D.h"
#include <algorithm>

namespace nv {

NVChart::NVChart(NVChartType type) : m_type(type) {
    m_theme = NVTheme::excel();
    m_xAxisConfig.position = NV_AXIS_BOTTOM;
    m_yAxisConfig.position = NV_AXIS_LEFT;
}

void NVChart::setData(NVDataStore* store) {
    m_data = store;
    m_layoutDirty = true;
}

void NVChart::setSeriesBinding(const NVSeriesBinding& binding) {
    m_series.clear();
    for (size_t i = 0; i < binding.yColumns.size(); i++) {
        NVSeries s;
        s.xColumn = binding.xColumn;
        s.yColumn = binding.yColumns[i];
        s.color = m_theme.seriesColors[i % m_theme.seriesColors.size()];
        s.lineWidth = m_theme.lineWidth;
        if (m_data) {
            s.name = m_data->columnName(binding.yColumns[i]);
        } else {
            s.name = "Series " + std::to_string(i + 1);
        }
        m_series.push_back(s);
    }
    m_layoutDirty = true;
}

void NVChart::addSeries(const NVSeries& series) {
    m_series.push_back(series);
    m_layoutDirty = true;
}

void NVChart::clearSeries() {
    m_series.clear();
    m_layoutDirty = true;
}

void NVChart::layout() {
    if (!m_data) return;

    // Calculate plot area (bounds minus margins)
    m_plotArea = {
        m_bounds.x + m_marginLeft,
        m_bounds.y + m_marginTop,
        m_bounds.width - m_marginLeft - m_marginRight,
        m_bounds.height - m_marginTop - m_marginBottom
    };

    // Compute data range
    double xMin, xMax, yMin, yMax;
    computeDataRange(xMin, xMax, yMin, yMax);

    // Configure and layout axes
    m_xAxis.configure(m_xAxisConfig);
    m_xAxis.setDataRange(xMin, xMax);
    m_xAxis.layout(m_plotArea.width, m_plotArea.y + m_plotArea.height);

    m_yAxis.configure(m_yAxisConfig);
    m_yAxis.setDataRange(yMin, yMax);
    m_yAxis.layout(m_plotArea.height, m_plotArea.x);

    // Setup legend
    std::vector<NVLegendItem> legendItems;
    for (const auto& s : m_series) {
        legendItems.push_back({s.name, s.color, s.visible});
    }
    m_legend.setItems(legendItems);

    m_layoutDirty = false;
    if (m_data) m_lastDataVersion = m_data->version();
}

void NVChart::render(NVRenderer2D& renderer) {
    if (!m_data) return;
    if (isDirty()) layout();

    // Background
    renderer.drawRect(m_bounds, m_theme.backgroundColor);

    // Plot area background
    renderer.drawRect(m_plotArea, m_theme.plotAreaColor);

    // Axes
    m_xAxis.render(renderer, m_theme, m_plotArea);
    m_yAxis.render(renderer, m_theme, m_plotArea);

    // Title
    if (!m_title.empty()) {
        float titleX = m_bounds.x + m_bounds.width * 0.5f;
        float titleY = m_bounds.y + m_marginTop * 0.5f;
        renderer.drawText(m_title.c_str(), {titleX, titleY}, m_theme.titleColor, m_theme.titleFontSize);
    }

    // Chart-specific rendering
    renderChart(renderer);

    // Legend
    m_legend.render(renderer, m_theme, m_plotArea);
}

void NVChart::computeDataRange(double& xMin, double& xMax, double& yMin, double& yMax) {
    xMin = m_data->minValue(m_series.empty() ? 0 : m_series[0].xColumn);
    xMax = m_data->maxValue(m_series.empty() ? 0 : m_series[0].xColumn);
    yMin = std::numeric_limits<double>::max();
    yMax = std::numeric_limits<double>::lowest();

    for (const auto& s : m_series) {
        if (!s.visible) continue;
        double sMin = m_data->minValue(s.yColumn);
        double sMax = m_data->maxValue(s.yColumn);
        yMin = std::min(yMin, sMin);
        yMax = std::max(yMax, sMax);
    }

    if (yMin > yMax) { yMin = 0; yMax = 1; }
}

NVChart::HitResult NVChart::hitTest(NVPoint /*point*/) const {
    return {}; // Override in subclasses
}

} // namespace nv
