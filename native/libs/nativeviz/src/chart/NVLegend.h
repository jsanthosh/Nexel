#ifndef NV_LEGEND_H
#define NV_LEGEND_H

#include "nativeviz/nv_types.h"
#include "NVTheme.h"
#include <vector>
#include <string>

namespace nv {

class NVRenderer2D;

enum class NVLegendPosition {
    TopRight,
    TopLeft,
    BottomRight,
    BottomLeft,
    Right,
    Bottom,
    None
};

struct NVLegendItem {
    std::string label;
    NVColor color;
    bool visible = true;
};

class NVLegend {
public:
    NVLegend();

    void setPosition(NVLegendPosition pos) { m_position = pos; }
    void setItems(const std::vector<NVLegendItem>& items) { m_items = items; }
    void addItem(const std::string& label, NVColor color);
    void clear() { m_items.clear(); }

    // Calculate the space the legend needs (width or height)
    NVSize calculateSize(const NVTheme& theme) const;

    // Render the legend
    void render(NVRenderer2D& renderer, const NVTheme& theme, NVRect chartBounds) const;

    // Hit test: returns index of clicked legend item, or -1
    int hitTest(NVPoint point, NVRect chartBounds, const NVTheme& theme) const;

    NVLegendPosition position() const { return m_position; }
    const std::vector<NVLegendItem>& items() const { return m_items; }

private:
    NVLegendPosition m_position = NVLegendPosition::TopRight;
    std::vector<NVLegendItem> m_items;

    NVRect legendRect(NVRect chartBounds, NVSize legendSize) const;
};

} // namespace nv

#endif // NV_LEGEND_H
