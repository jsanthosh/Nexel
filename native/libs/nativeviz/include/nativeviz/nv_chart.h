#ifndef NV_CHART_API_H
#define NV_CHART_API_H

/**
 * NativeViz — Chart-specific C API extensions
 */

#include "nv.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Line Chart ---
void nv_line_chart_set_smooth(NVChartRef chart, int smooth);
void nv_line_chart_set_fill_area(NVChartRef chart, int fill, float opacity);
void nv_line_chart_set_markers(NVChartRef chart, int show, NVMarkerShape shape);

// --- Bar Chart ---
void nv_bar_chart_set_mode(NVChartRef chart, int mode); // 0=grouped, 1=stacked, 2=stacked100
void nv_bar_chart_set_direction(NVChartRef chart, int direction); // 0=vertical, 1=horizontal
void nv_bar_chart_set_corner_radius(NVChartRef chart, float radius);
void nv_bar_chart_show_labels(NVChartRef chart, int show);

// --- Scatter Chart ---
void nv_scatter_chart_set_marker(NVChartRef chart, NVMarkerShape shape, float size);
void nv_scatter_chart_set_size_mapping(NVChartRef chart, int column, float minSize, float maxSize);
void nv_scatter_chart_show_trend_line(NVChartRef chart, int show);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NV_CHART_API_H
