#ifndef NV_BAR_CHART_H
#define NV_BAR_CHART_H

#include "../NVChart.h"

namespace nv {

enum class NVBarMode {
    Grouped,    // side-by-side bars
    Stacked,    // stacked bars
    Stacked100  // 100% stacked bars
};

enum class NVBarDirection {
    Vertical,
    Horizontal
};

struct NVBarChartConfig {
    NVBarMode mode = NVBarMode::Grouped;
    NVBarDirection direction = NVBarDirection::Vertical;
    float barWidthFraction = 0.7f; // fraction of category width
    float cornerRadius = 2.0f;
    bool showLabels = false;      // value labels on bars
};

class NVBarChart : public NVChart {
public:
    NVBarChart();

    void setConfig(const NVBarChartConfig& config) { m_config = config; }
    const NVBarChartConfig& config() const { return m_config; }

    HitResult hitTest(NVPoint point) const override;

protected:
    void renderChart(NVRenderer2D& renderer) override;
    void computeDataRange(double& xMin, double& xMax, double& yMin, double& yMax) override;

private:
    NVBarChartConfig m_config;

    struct BarRect {
        NVRect rect;
        int seriesIndex;
        size_t pointIndex;
        NVColor color;
    };

    std::vector<BarRect> computeBarRects() const;
};

} // namespace nv

#endif // NV_BAR_CHART_H
