#ifndef NV_LINE_CHART_H
#define NV_LINE_CHART_H

#include "../NVChart.h"

namespace nv {

struct NVLineChartConfig {
    bool showMarkers = true;
    bool fillArea = false;        // fill between line and axis
    float fillOpacity = 0.15f;
    bool smoothCurve = false;     // Catmull-Rom interpolation
    float tension = 0.5f;         // smoothing tension (0-1)
};

class NVLineChart : public NVChart {
public:
    NVLineChart();

    void setConfig(const NVLineChartConfig& config) { m_config = config; }
    const NVLineChartConfig& config() const { return m_config; }

    HitResult hitTest(NVPoint point) const override;

protected:
    void renderChart(NVRenderer2D& renderer) override;

private:
    NVLineChartConfig m_config;

    // Convert data points to screen coordinates
    std::vector<NVPoint> dataToScreen(const NVSeries& series) const;

    // Catmull-Rom interpolation
    std::vector<NVPoint> smoothPoints(const std::vector<NVPoint>& points) const;

    // Render area fill
    void renderAreaFill(NVRenderer2D& renderer, const std::vector<NVPoint>& screenPoints, NVColor color) const;
};

} // namespace nv

#endif // NV_LINE_CHART_H
