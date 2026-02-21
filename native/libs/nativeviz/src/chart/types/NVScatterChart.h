#ifndef NV_SCATTER_CHART_H
#define NV_SCATTER_CHART_H

#include "../NVChart.h"

namespace nv {

struct NVScatterChartConfig {
    NVMarkerShape markerShape = NV_MARKER_CIRCLE;
    float markerSize = 6.0f;
    float markerOpacity = 0.8f;
    bool sizeMapping = false;    // map size to a third column
    int sizeColumn = -1;
    float minMarkerSize = 3.0f;
    float maxMarkerSize = 20.0f;
    bool colorMapping = false;   // map color to a fourth column
    int colorColumn = -1;
    bool showTrendLine = false;
};

class NVScatterChart : public NVChart {
public:
    NVScatterChart();

    void setConfig(const NVScatterChartConfig& config) { m_config = config; }
    const NVScatterChartConfig& config() const { return m_config; }

    HitResult hitTest(NVPoint point) const override;

protected:
    void renderChart(NVRenderer2D& renderer) override;

private:
    NVScatterChartConfig m_config;

    void renderTrendLine(NVRenderer2D& renderer, const NVSeries& series) const;
};

} // namespace nv

#endif // NV_SCATTER_CHART_H
