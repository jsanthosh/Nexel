#ifndef NV_CHART_H
#define NV_CHART_H

#include "nativeviz/nv_types.h"
#include "NVAxis.h"
#include "NVLegend.h"
#include "NVTheme.h"
#include "../data/NVDataStore.h"
#include <memory>
#include <vector>
#include <string>

namespace nv {

class NVRenderer2D;

struct NVSeriesBinding {
    int xColumn = 0;           // column index for X values
    std::vector<int> yColumns; // column indices for Y series
};

struct NVSeries {
    std::string name;
    int xColumn = 0;
    int yColumn = 0;
    NVColor color;
    float lineWidth = 2.0f;
    NVMarkerShape marker = NV_MARKER_NONE;
    float markerSize = 4.0f;
    bool visible = true;
};

class NVChart {
public:
    explicit NVChart(NVChartType type);
    virtual ~NVChart() = default;

    // Data binding
    void setData(NVDataStore* store);
    void setSeriesBinding(const NVSeriesBinding& binding);
    void addSeries(const NVSeries& series);
    void clearSeries();

    // Configuration
    void setTitle(const std::string& title) { m_title = title; }
    void setTheme(const NVTheme& theme) { m_theme = theme; }
    void setBounds(NVRect bounds) { m_bounds = bounds; m_layoutDirty = true; }
    void setXAxisConfig(const NVAxisConfig& config) { m_xAxisConfig = config; m_layoutDirty = true; }
    void setYAxisConfig(const NVAxisConfig& config) { m_yAxisConfig = config; m_layoutDirty = true; }
    void setLegendPosition(NVLegendPosition pos) { m_legend.setPosition(pos); }

    // Layout and rendering
    virtual void layout();
    virtual void render(NVRenderer2D& renderer);

    // Hit testing
    struct HitResult {
        int seriesIndex = -1;
        size_t pointIndex = 0;
        double xValue = 0;
        double yValue = 0;
        NVPoint screenPos;
    };
    virtual HitResult hitTest(NVPoint point) const;

    // Accessors
    NVChartType chartType() const { return m_type; }
    NVRect bounds() const { return m_bounds; }
    NVRect plotArea() const { return m_plotArea; }
    const NVDataStore* data() const { return m_data; }
    bool isDirty() const { return m_layoutDirty || (m_data && m_data->version() != m_lastDataVersion); }
    void markClean() { m_layoutDirty = false; if (m_data) m_lastDataVersion = m_data->version(); }

protected:
    // Subclasses implement this
    virtual void renderChart(NVRenderer2D& renderer) = 0;
    virtual void computeDataRange(double& xMin, double& xMax, double& yMin, double& yMax);

    NVChartType m_type;
    NVDataStore* m_data = nullptr;
    NVRect m_bounds = {0, 0, 400, 300};
    NVRect m_plotArea;
    std::string m_title;
    NVTheme m_theme;

    NVAxis m_xAxis;
    NVAxis m_yAxis;
    NVAxisConfig m_xAxisConfig;
    NVAxisConfig m_yAxisConfig;
    NVLegend m_legend;

    std::vector<NVSeries> m_series;

    bool m_layoutDirty = true;
    uint64_t m_lastDataVersion = 0;

    // Margins
    float m_marginTop = 30;
    float m_marginRight = 20;
    float m_marginBottom = 40;
    float m_marginLeft = 50;
};

} // namespace nv

#endif // NV_CHART_H
