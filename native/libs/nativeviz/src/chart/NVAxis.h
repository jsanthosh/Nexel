#ifndef NV_AXIS_H
#define NV_AXIS_H

#include "nativeviz/nv_types.h"
#include "NVTheme.h"
#include <vector>
#include <string>

namespace nv {

class NVRenderer2D;

struct NVTick {
    double value;
    std::string label;
    float position;    // pixel position along axis
};

struct NVAxisConfig {
    NVAxisType type = NV_AXIS_LINEAR;
    NVAxisPosition position = NV_AXIS_BOTTOM;
    std::string title;

    // Range (auto-calculated if min >= max)
    double minValue = 0;
    double maxValue = 0;
    bool autoRange = true;

    // Ticks
    int desiredTickCount = 5;
    bool showGrid = true;
    bool showLabels = true;
    bool showTitle = true;

    // Category axis: explicit labels
    std::vector<std::string> categories;

    // Number formatting
    int decimalPlaces = -1; // -1 = auto
    std::string prefix;      // e.g. "$"
    std::string suffix;      // e.g. "%"
    bool useKMB = true;      // K/M/B suffixes for large numbers
};

class NVAxis {
public:
    NVAxis();

    void configure(const NVAxisConfig& config);
    void setDataRange(double min, double max);
    void layout(float axisLength, float perpOffset);

    // Convert data value to pixel position
    float dataToPixel(double value) const;
    // Convert pixel position to data value
    double pixelToData(float pixel) const;

    // Render axis line, ticks, labels, grid
    void render(NVRenderer2D& renderer, const NVTheme& theme, NVRect plotArea) const;

    const std::vector<NVTick>& ticks() const { return m_ticks; }
    float axisLength() const { return m_axisLength; }
    double effectiveMin() const { return m_effectiveMin; }
    double effectiveMax() const { return m_effectiveMax; }

    // Space needed for labels (width for Y axis, height for X axis)
    float labelSpace() const { return m_labelSpace; }

private:
    NVAxisConfig m_config;

    // Computed state
    double m_effectiveMin = 0;
    double m_effectiveMax = 1;
    double m_tickStep = 0.2;
    float m_axisLength = 0;
    float m_perpOffset = 0;
    float m_labelSpace = 40;

    std::vector<NVTick> m_ticks;

    // Wilkinson's extended tick algorithm
    void generateNiceTicks();
    void generateCategoryTicks();
    void generateLogTicks();

    std::string formatValue(double value) const;
    static double niceNumber(double value, bool round);
};

} // namespace nv

#endif // NV_AXIS_H
