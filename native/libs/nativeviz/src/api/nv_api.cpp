#include "nativeviz/nv.h"
#include "nativeviz/nv_chart.h"
#include "../hal/NVDevice.h"
#include "../render/NVRenderer2D.h"
#include "../data/NVDataStore.h"
#include "../scene/NVScene.h"
#include "../chart/NVChart.h"
#include "../chart/NVTheme.h"
#include "../chart/types/NVLineChart.h"
#include "../chart/types/NVBarChart.h"
#include "../chart/types/NVScatterChart.h"
#include <string>

// Internal context structure
struct NVContext_s {
    std::unique_ptr<nv::NVDevice> device;
    std::unique_ptr<nv::NVRenderer2D> renderer;
    std::string deviceNameStr;
};

struct NVScene_s {
    nv::NVScene scene;
    NVContext_s* ctx;
    NVScene_s(nv::NVDevice* device, NVContext_s* c) : scene(device), ctx(c) {}
};

struct NVChart_s {
    nv::NVChart* chart; // owned by NVScene
};

struct NVDataStore_s {
    nv::NVDataStore store;
};

// --- Context ---

NVContextRef nv_context_create(NVBackend backend) {
    auto ctx = new NVContext_s();
    ctx->device = nv::NVDevice::create(backend);
    if (!ctx->device) {
        delete ctx;
        return nullptr;
    }
    ctx->renderer = std::make_unique<nv::NVRenderer2D>(ctx->device.get());
    ctx->deviceNameStr = ctx->device->deviceName();
    return ctx;
}

void nv_context_destroy(NVContextRef ctx) {
    delete ctx;
}

const char* nv_context_device_name(NVContextRef ctx) {
    return ctx ? ctx->deviceNameStr.c_str() : "";
}

// --- Data Store ---

NVDataStoreRef nv_data_create(void) {
    return new NVDataStore_s();
}

void nv_data_destroy(NVDataStoreRef data) {
    delete data;
}

int nv_data_add_column(NVDataStoreRef data, const char* name, int type) {
    if (!data) return -1;
    nv::NVColumnType colType;
    switch (type) {
        case 0: colType = nv::NVColumnType::Float64; break;
        case 1: colType = nv::NVColumnType::String; break;
        case 2: colType = nv::NVColumnType::DateTime; break;
        default: colType = nv::NVColumnType::Float64; break;
    }
    return data->store.addColumn(name ? name : "", colType);
}

void nv_data_set_float64(NVDataStoreRef data, int column, const double* values, size_t count) {
    if (data && values) {
        data->store.setFloat64Column(column, values, count);
    }
}

void nv_data_append_float64(NVDataStoreRef data, int column, const double* values, size_t count) {
    if (data && values) {
        data->store.appendFloat64(column, values, count);
    }
}

size_t nv_data_row_count(NVDataStoreRef data) {
    return data ? data->store.rowCount() : 0;
}

// --- Scene ---

NVSceneRef nv_scene_create(NVContextRef ctx) {
    if (!ctx) return nullptr;
    return new NVScene_s(ctx->device.get(), ctx);
}

void nv_scene_destroy(NVSceneRef scene) {
    delete scene;
}

NVChartRef nv_scene_add_chart(NVSceneRef scene, NVChartType type) {
    if (!scene) return nullptr;
    auto* chart = scene->scene.addChart(type);
    if (!chart) return nullptr;
    auto* ref = new NVChart_s();
    ref->chart = chart;
    return ref;
}

void nv_scene_remove_chart(NVSceneRef scene, int index) {
    if (scene) scene->scene.removeChart(index);
}

NVChartRef nv_scene_get_chart(NVSceneRef scene, int index) {
    if (!scene) return nullptr;
    auto* chart = scene->scene.getChart(index);
    if (!chart) return nullptr;
    auto* ref = new NVChart_s();
    ref->chart = chart;
    return ref;
}

int nv_scene_chart_count(NVSceneRef scene) {
    return scene ? scene->scene.chartCount() : 0;
}

void nv_scene_render(NVSceneRef scene, float width, float height) {
    if (!scene || !scene->ctx || !scene->ctx->renderer) return;
    // TODO: create offscreen texture and render to it
    scene->scene.renderAll(*scene->ctx->renderer, width, height);
}

void nv_scene_pan(NVSceneRef scene, float dx, float dy) {
    if (scene) scene->scene.pan(dx, dy);
}

void nv_scene_zoom(NVSceneRef scene, float scale, float cx, float cy) {
    if (scene) scene->scene.zoom(scale, cx, cy);
}

// --- Chart ---

void nv_chart_set_data(NVChartRef chart, NVDataStoreRef data) {
    if (chart && chart->chart && data) {
        chart->chart->setData(&data->store);
    }
}

void nv_chart_set_bounds(NVChartRef chart, float x, float y, float w, float h) {
    if (chart && chart->chart) {
        chart->chart->setBounds({x, y, w, h});
    }
}

void nv_chart_set_title(NVChartRef chart, const char* title) {
    if (chart && chart->chart && title) {
        chart->chart->setTitle(title);
    }
}

void nv_chart_set_theme(NVChartRef chart, int themeIndex) {
    if (!chart || !chart->chart) return;
    nv::NVTheme theme;
    switch (themeIndex) {
        case 0: theme = nv::NVTheme::excel(); break;
        case 1: theme = nv::NVTheme::material(); break;
        case 2: theme = nv::NVTheme::solarized(); break;
        case 3: theme = nv::NVTheme::dark(); break;
        case 4: theme = nv::NVTheme::monochrome(); break;
        case 5: theme = nv::NVTheme::pastel(); break;
        default: theme = nv::NVTheme::excel(); break;
    }
    chart->chart->setTheme(theme);
}

void nv_chart_add_series(NVChartRef chart, int xColumn, int yColumn, const char* name) {
    if (!chart || !chart->chart) return;
    nv::NVSeries series;
    series.xColumn = xColumn;
    series.yColumn = yColumn;
    series.name = name ? name : "";
    chart->chart->addSeries(series);
}

void nv_chart_set_x_axis(NVChartRef chart, int type, const char* title) {
    if (!chart || !chart->chart) return;
    nv::NVAxisConfig config;
    config.type = static_cast<NVAxisType>(type);
    config.position = NV_AXIS_BOTTOM;
    config.title = title ? title : "";
    chart->chart->setXAxisConfig(config);
}

void nv_chart_set_y_axis(NVChartRef chart, int type, const char* title) {
    if (!chart || !chart->chart) return;
    nv::NVAxisConfig config;
    config.type = static_cast<NVAxisType>(type);
    config.position = NV_AXIS_LEFT;
    config.title = title ? title : "";
    chart->chart->setYAxisConfig(config);
}

void nv_scene_render_to_buffer(NVSceneRef /*scene*/, float /*width*/, float /*height*/,
                                void* /*pixelBuffer*/, size_t /*bufferSize*/) {
    // TODO: implement offscreen rendering + readback
}

// --- Line Chart extensions ---

void nv_line_chart_set_smooth(NVChartRef chart, int smooth) {
    if (!chart || !chart->chart) return;
    if (auto* lc = dynamic_cast<nv::NVLineChart*>(chart->chart)) {
        auto config = lc->config();
        config.smoothCurve = (smooth != 0);
        lc->setConfig(config);
    }
}

void nv_line_chart_set_fill_area(NVChartRef chart, int fill, float opacity) {
    if (!chart || !chart->chart) return;
    if (auto* lc = dynamic_cast<nv::NVLineChart*>(chart->chart)) {
        auto config = lc->config();
        config.fillArea = (fill != 0);
        config.fillOpacity = opacity;
        lc->setConfig(config);
    }
}

void nv_line_chart_set_markers(NVChartRef chart, int show, NVMarkerShape shape) {
    if (!chart || !chart->chart) return;
    if (auto* lc = dynamic_cast<nv::NVLineChart*>(chart->chart)) {
        auto config = lc->config();
        config.showMarkers = (show != 0);
        lc->setConfig(config);
        (void)shape; // marker shape is per-series, not chart-wide
    }
}

// --- Bar Chart extensions ---

void nv_bar_chart_set_mode(NVChartRef chart, int mode) {
    if (!chart || !chart->chart) return;
    if (auto* bc = dynamic_cast<nv::NVBarChart*>(chart->chart)) {
        auto config = bc->config();
        config.mode = static_cast<nv::NVBarMode>(mode);
        bc->setConfig(config);
    }
}

void nv_bar_chart_set_direction(NVChartRef chart, int direction) {
    if (!chart || !chart->chart) return;
    if (auto* bc = dynamic_cast<nv::NVBarChart*>(chart->chart)) {
        auto config = bc->config();
        config.direction = static_cast<nv::NVBarDirection>(direction);
        bc->setConfig(config);
    }
}

void nv_bar_chart_set_corner_radius(NVChartRef chart, float radius) {
    if (!chart || !chart->chart) return;
    if (auto* bc = dynamic_cast<nv::NVBarChart*>(chart->chart)) {
        auto config = bc->config();
        config.cornerRadius = radius;
        bc->setConfig(config);
    }
}

void nv_bar_chart_show_labels(NVChartRef chart, int show) {
    if (!chart || !chart->chart) return;
    if (auto* bc = dynamic_cast<nv::NVBarChart*>(chart->chart)) {
        auto config = bc->config();
        config.showLabels = (show != 0);
        bc->setConfig(config);
    }
}

// --- Scatter Chart extensions ---

void nv_scatter_chart_set_marker(NVChartRef chart, NVMarkerShape shape, float size) {
    if (!chart || !chart->chart) return;
    if (auto* sc = dynamic_cast<nv::NVScatterChart*>(chart->chart)) {
        auto config = sc->config();
        config.markerShape = shape;
        config.markerSize = size;
        sc->setConfig(config);
    }
}

void nv_scatter_chart_set_size_mapping(NVChartRef chart, int column, float minSize, float maxSize) {
    if (!chart || !chart->chart) return;
    if (auto* sc = dynamic_cast<nv::NVScatterChart*>(chart->chart)) {
        auto config = sc->config();
        config.sizeMapping = true;
        config.sizeColumn = column;
        config.minMarkerSize = minSize;
        config.maxMarkerSize = maxSize;
        sc->setConfig(config);
    }
}

void nv_scatter_chart_show_trend_line(NVChartRef chart, int show) {
    if (!chart || !chart->chart) return;
    if (auto* sc = dynamic_cast<nv::NVScatterChart*>(chart->chart)) {
        auto config = sc->config();
        config.showTrendLine = (show != 0);
        sc->setConfig(config);
    }
}
