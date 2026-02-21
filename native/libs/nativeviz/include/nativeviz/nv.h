#ifndef NV_H
#define NV_H

/**
 * NativeViz — High-Performance Data Visualization Library
 *
 * C API for cross-language integration.
 * All functions are thread-safe unless otherwise noted.
 */

#include "nv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Opaque handles ---
typedef struct NVContext_s* NVContextRef;
typedef struct NVScene_s* NVSceneRef;
typedef struct NVChart_s* NVChartRef;
typedef struct NVDataStore_s* NVDataStoreRef;

// --- Context ---
NVContextRef nv_context_create(NVBackend backend);
void nv_context_destroy(NVContextRef ctx);
const char* nv_context_device_name(NVContextRef ctx);

// --- Data Store ---
NVDataStoreRef nv_data_create(void);
void nv_data_destroy(NVDataStoreRef data);
int nv_data_add_column(NVDataStoreRef data, const char* name, int type); // type: 0=float64, 1=string, 2=datetime
void nv_data_set_float64(NVDataStoreRef data, int column, const double* values, size_t count);
void nv_data_append_float64(NVDataStoreRef data, int column, const double* values, size_t count);
size_t nv_data_row_count(NVDataStoreRef data);

// --- Scene ---
NVSceneRef nv_scene_create(NVContextRef ctx);
void nv_scene_destroy(NVSceneRef scene);

NVChartRef nv_scene_add_chart(NVSceneRef scene, NVChartType type);
void nv_scene_remove_chart(NVSceneRef scene, int index);
NVChartRef nv_scene_get_chart(NVSceneRef scene, int index);
int nv_scene_chart_count(NVSceneRef scene);

void nv_scene_render(NVSceneRef scene, float width, float height);
void nv_scene_pan(NVSceneRef scene, float dx, float dy);
void nv_scene_zoom(NVSceneRef scene, float scale, float cx, float cy);

// --- Chart ---
void nv_chart_set_data(NVChartRef chart, NVDataStoreRef data);
void nv_chart_set_bounds(NVChartRef chart, float x, float y, float w, float h);
void nv_chart_set_title(NVChartRef chart, const char* title);
void nv_chart_set_theme(NVChartRef chart, int themeIndex); // 0=Excel, 1=Material, 2=Solarized, 3=Dark, 4=Mono, 5=Pastel
void nv_chart_add_series(NVChartRef chart, int xColumn, int yColumn, const char* name);
void nv_chart_set_x_axis(NVChartRef chart, int type, const char* title); // type: NVAxisType
void nv_chart_set_y_axis(NVChartRef chart, int type, const char* title);

// --- Rendering to texture ---
// For offscreen rendering (e.g., export to image)
void nv_scene_render_to_buffer(NVSceneRef scene, float width, float height,
                                void* pixelBuffer, size_t bufferSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // NV_H
