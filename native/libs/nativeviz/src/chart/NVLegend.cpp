#include "NVLegend.h"
#include "../render/NVRenderer2D.h"

namespace nv {

NVLegend::NVLegend() = default;

void NVLegend::addItem(const std::string& label, NVColor color) {
    m_items.push_back({label, color, true});
}

NVSize NVLegend::calculateSize(const NVTheme& theme) const {
    if (m_items.empty() || m_position == NVLegendPosition::None) {
        return {0, 0};
    }

    float itemHeight = theme.legendFontSize + 4;
    float totalHeight = 8 + m_items.size() * itemHeight + 4; // padding
    float maxWidth = 0;

    for (const auto& item : m_items) {
        // Estimate text width: ~7px per character at 11pt
        float textWidth = item.label.length() * (theme.legendFontSize * 0.6f);
        float itemWidth = 20 + 6 + textWidth; // swatch + gap + text
        maxWidth = std::max(maxWidth, itemWidth);
    }

    return {maxWidth + 16, totalHeight}; // padding
}

NVRect NVLegend::legendRect(NVRect chartBounds, NVSize legendSize) const {
    float margin = 10;
    float x = 0, y = 0;

    switch (m_position) {
        case NVLegendPosition::TopRight:
            x = chartBounds.x + chartBounds.width - legendSize.width - margin;
            y = chartBounds.y + margin;
            break;
        case NVLegendPosition::TopLeft:
            x = chartBounds.x + margin;
            y = chartBounds.y + margin;
            break;
        case NVLegendPosition::BottomRight:
            x = chartBounds.x + chartBounds.width - legendSize.width - margin;
            y = chartBounds.y + chartBounds.height - legendSize.height - margin;
            break;
        case NVLegendPosition::BottomLeft:
            x = chartBounds.x + margin;
            y = chartBounds.y + chartBounds.height - legendSize.height - margin;
            break;
        case NVLegendPosition::Right:
            x = chartBounds.x + chartBounds.width - legendSize.width - margin;
            y = chartBounds.y + (chartBounds.height - legendSize.height) * 0.5f;
            break;
        case NVLegendPosition::Bottom:
            x = chartBounds.x + (chartBounds.width - legendSize.width) * 0.5f;
            y = chartBounds.y + chartBounds.height - legendSize.height - margin;
            break;
        case NVLegendPosition::None:
            return {0, 0, 0, 0};
    }

    return {x, y, legendSize.width, legendSize.height};
}

void NVLegend::render(NVRenderer2D& renderer, const NVTheme& theme, NVRect chartBounds) const {
    if (m_items.empty() || m_position == NVLegendPosition::None) return;

    NVSize size = calculateSize(theme);
    NVRect rect = legendRect(chartBounds, size);

    // Background with border
    ShapeParams bg;
    bg.type = NV_SHAPE_ROUNDED_RECT;
    bg.bounds = rect;
    bg.fillColor = theme.legendBackgroundColor;
    bg.strokeColor = theme.legendBorderColor;
    bg.strokeWidth = 1;
    bg.cornerRadius = 4;
    renderer.drawShape(bg);

    // Items
    float itemHeight = theme.legendFontSize + 4;
    float y = rect.y + 8;
    float x = rect.x + 8;

    for (const auto& item : m_items) {
        // Color swatch
        float swatchSize = theme.legendFontSize * 0.8f;
        renderer.drawRect({x, y + 1, swatchSize, swatchSize},
                         item.visible ? item.color : NVColor{0.7f, 0.7f, 0.7f, 1}, 2);

        // Label
        renderer.drawText(item.label.c_str(), {x + swatchSize + 6, y + swatchSize * 0.5f},
                         item.visible ? theme.axisLabelColor : NVColor{0.7f, 0.7f, 0.7f, 1},
                         theme.legendFontSize);

        y += itemHeight;
    }
}

int NVLegend::hitTest(NVPoint point, NVRect chartBounds, const NVTheme& theme) const {
    if (m_items.empty() || m_position == NVLegendPosition::None) return -1;

    NVSize size = calculateSize(theme);
    NVRect rect = legendRect(chartBounds, size);

    // Check if point is inside legend
    if (point.x < rect.x || point.x > rect.x + rect.width ||
        point.y < rect.y || point.y > rect.y + rect.height) {
        return -1;
    }

    float itemHeight = theme.legendFontSize + 4;
    float relY = point.y - rect.y - 8;
    int idx = static_cast<int>(relY / itemHeight);
    if (idx >= 0 && idx < static_cast<int>(m_items.size())) {
        return idx;
    }
    return -1;
}

} // namespace nv
