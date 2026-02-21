#ifndef NV_SCENE_H
#define NV_SCENE_H

#include "nativeviz/nv_types.h"
#include "../chart/NVChart.h"
#include <vector>
#include <memory>
#include <unordered_set>

namespace nv {

class NVRenderer2D;
class NVDevice;
class NVTexture;

struct NVChartNode {
    std::unique_ptr<NVChart> chart;
    NVRect bounds;           // world-space bounds
    bool dirty = true;
    int cacheTextureIdx = -1;
};

// Spatial grid for viewport culling of 10K+ charts
class NVSpatialGrid {
public:
    NVSpatialGrid(float worldWidth = 10000, float worldHeight = 10000, int cellsX = 256, int cellsY = 256);

    void clear();
    void insert(int nodeIndex, NVRect bounds);
    void remove(int nodeIndex, NVRect bounds);
    std::vector<int> query(NVRect viewport) const;

private:
    float m_worldWidth, m_worldHeight;
    int m_cellsX, m_cellsY;
    float m_cellWidth, m_cellHeight;

    // Each cell stores a set of node indices
    std::vector<std::unordered_set<int>> m_cells;

    void getCellRange(NVRect bounds, int& minCX, int& minCY, int& maxCX, int& maxCY) const;
};

class NVScene {
public:
    explicit NVScene(NVDevice* device);
    ~NVScene();

    // Chart management
    NVChart* addChart(NVChartType type);
    void removeChart(int index);
    NVChart* getChart(int index);
    int chartCount() const { return static_cast<int>(m_nodes.size()); }

    // Rendering
    void render(NVRenderer2D& renderer, NVViewport viewport);
    void renderAll(NVRenderer2D& renderer, float width, float height);

    // Viewport
    void setViewport(NVViewport viewport) { m_viewport = viewport; }
    NVViewport viewport() const { return m_viewport; }

    // Pan/zoom
    void pan(float dx, float dy);
    void zoom(float scale, float centerX, float centerY);
    float zoomLevel() const { return m_zoom; }

    // Hit testing
    struct SceneHitResult {
        int chartIndex = -1;
        NVChart::HitResult chartHit;
    };
    SceneHitResult hitTest(NVPoint screenPoint) const;

    // Force re-render all charts
    void invalidateAll();

private:
    NVDevice* m_device;
    std::vector<NVChartNode> m_nodes;
    NVSpatialGrid m_grid;

    NVViewport m_viewport = {0, 0, 800, 600};
    float m_panX = 0, m_panY = 0;
    float m_zoom = 1.0f;

    void updateGrid();
    NVPoint screenToWorld(NVPoint screen) const;
    NVRect worldToScreen(NVRect world) const;
};

} // namespace nv

#endif // NV_SCENE_H
