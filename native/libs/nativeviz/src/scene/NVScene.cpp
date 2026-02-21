#include "NVScene.h"
#include "../chart/NVChart.h"
#include "../chart/types/NVLineChart.h"
#include "../chart/types/NVBarChart.h"
#include "../chart/types/NVScatterChart.h"
#include "../render/NVRenderer2D.h"
#include "../hal/NVDevice.h"
#include <algorithm>

namespace nv {

// --- NVSpatialGrid ---

NVSpatialGrid::NVSpatialGrid(float worldWidth, float worldHeight, int cellsX, int cellsY)
    : m_worldWidth(worldWidth), m_worldHeight(worldHeight),
      m_cellsX(cellsX), m_cellsY(cellsY) {
    m_cellWidth = worldWidth / cellsX;
    m_cellHeight = worldHeight / cellsY;
    m_cells.resize(cellsX * cellsY);
}

void NVSpatialGrid::clear() {
    for (auto& cell : m_cells) cell.clear();
}

void NVSpatialGrid::getCellRange(NVRect bounds, int& minCX, int& minCY, int& maxCX, int& maxCY) const {
    minCX = std::max(0, static_cast<int>(bounds.x / m_cellWidth));
    minCY = std::max(0, static_cast<int>(bounds.y / m_cellHeight));
    maxCX = std::min(m_cellsX - 1, static_cast<int>((bounds.x + bounds.width) / m_cellWidth));
    maxCY = std::min(m_cellsY - 1, static_cast<int>((bounds.y + bounds.height) / m_cellHeight));
}

void NVSpatialGrid::insert(int nodeIndex, NVRect bounds) {
    int minCX, minCY, maxCX, maxCY;
    getCellRange(bounds, minCX, minCY, maxCX, maxCY);
    for (int cy = minCY; cy <= maxCY; cy++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            m_cells[cy * m_cellsX + cx].insert(nodeIndex);
        }
    }
}

void NVSpatialGrid::remove(int nodeIndex, NVRect bounds) {
    int minCX, minCY, maxCX, maxCY;
    getCellRange(bounds, minCX, minCY, maxCX, maxCY);
    for (int cy = minCY; cy <= maxCY; cy++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            m_cells[cy * m_cellsX + cx].erase(nodeIndex);
        }
    }
}

std::vector<int> NVSpatialGrid::query(NVRect viewport) const {
    int minCX, minCY, maxCX, maxCY;
    getCellRange(viewport, minCX, minCY, maxCX, maxCY);

    std::unordered_set<int> unique;
    for (int cy = minCY; cy <= maxCY; cy++) {
        for (int cx = minCX; cx <= maxCX; cx++) {
            const auto& cell = m_cells[cy * m_cellsX + cx];
            unique.insert(cell.begin(), cell.end());
        }
    }

    return std::vector<int>(unique.begin(), unique.end());
}

// --- NVScene ---

NVScene::NVScene(NVDevice* device) : m_device(device) {}

NVScene::~NVScene() = default;

NVChart* NVScene::addChart(NVChartType type) {
    NVChartNode node;
    switch (type) {
        case NV_CHART_LINE:
            node.chart = std::make_unique<NVLineChart>();
            break;
        case NV_CHART_BAR:
            node.chart = std::make_unique<NVBarChart>();
            break;
        case NV_CHART_SCATTER:
            node.chart = std::make_unique<NVScatterChart>();
            break;
        default:
            // Default to line chart for unimplemented types
            node.chart = std::make_unique<NVLineChart>();
            break;
    }

    node.bounds = node.chart->bounds();
    node.dirty = true;

    int idx = static_cast<int>(m_nodes.size());
    m_nodes.push_back(std::move(node));
    m_grid.insert(idx, m_nodes.back().bounds);

    return m_nodes.back().chart.get();
}

void NVScene::removeChart(int index) {
    if (index < 0 || index >= static_cast<int>(m_nodes.size())) return;
    m_grid.remove(index, m_nodes[index].bounds);
    m_nodes.erase(m_nodes.begin() + index);
    // Rebuild grid (indices shifted)
    updateGrid();
}

NVChart* NVScene::getChart(int index) {
    if (index < 0 || index >= static_cast<int>(m_nodes.size())) return nullptr;
    return m_nodes[index].chart.get();
}

void NVScene::render(NVRenderer2D& renderer, NVViewport viewport) {
    // Convert viewport to world space
    NVRect worldView = {
        viewport.x / m_zoom + m_panX,
        viewport.y / m_zoom + m_panY,
        viewport.width / m_zoom,
        viewport.height / m_zoom
    };

    // Query visible charts
    auto visible = m_grid.query(worldView);

    // Sort front-to-back (by index for now)
    std::sort(visible.begin(), visible.end());

    for (int idx : visible) {
        auto& node = m_nodes[idx];
        auto* chart = node.chart.get();

        // Transform chart bounds to screen space
        NVRect screenBounds = worldToScreen(node.bounds);
        chart->setBounds(screenBounds);

        // Render chart
        chart->render(renderer);
    }
}

void NVScene::renderAll(NVRenderer2D& renderer, float width, float height) {
    NVViewport vp = {0, 0, width, height};
    render(renderer, vp);
}

void NVScene::pan(float dx, float dy) {
    m_panX -= dx / m_zoom;
    m_panY -= dy / m_zoom;
}

void NVScene::zoom(float scale, float centerX, float centerY) {
    // Zoom toward center point
    float worldCX = centerX / m_zoom + m_panX;
    float worldCY = centerY / m_zoom + m_panY;

    m_zoom *= scale;
    m_zoom = std::clamp(m_zoom, 0.1f, 10.0f);

    m_panX = worldCX - centerX / m_zoom;
    m_panY = worldCY - centerY / m_zoom;
}

NVScene::SceneHitResult NVScene::hitTest(NVPoint screenPoint) const {
    SceneHitResult result;

    // Convert to world space
    NVPoint worldPt = screenToWorld(screenPoint);

    // Test charts in reverse order (top-most first)
    for (int i = static_cast<int>(m_nodes.size()) - 1; i >= 0; i--) {
        const auto& node = m_nodes[i];
        // Check AABB
        if (worldPt.x >= node.bounds.x && worldPt.x <= node.bounds.x + node.bounds.width &&
            worldPt.y >= node.bounds.y && worldPt.y <= node.bounds.y + node.bounds.height) {

            auto chartHit = node.chart->hitTest(screenPoint);
            if (chartHit.seriesIndex >= 0) {
                result.chartIndex = i;
                result.chartHit = chartHit;
                return result;
            }
        }
    }

    return result;
}

void NVScene::invalidateAll() {
    for (auto& node : m_nodes) {
        node.dirty = true;
    }
}

void NVScene::updateGrid() {
    m_grid.clear();
    for (int i = 0; i < static_cast<int>(m_nodes.size()); i++) {
        m_grid.insert(i, m_nodes[i].bounds);
    }
}

NVPoint NVScene::screenToWorld(NVPoint screen) const {
    return {
        screen.x / m_zoom + m_panX,
        screen.y / m_zoom + m_panY
    };
}

NVRect NVScene::worldToScreen(NVRect world) const {
    return {
        (world.x - m_panX) * m_zoom,
        (world.y - m_panY) * m_zoom,
        world.width * m_zoom,
        world.height * m_zoom
    };
}

} // namespace nv
