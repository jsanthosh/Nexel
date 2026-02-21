#ifndef NV_THEME_H
#define NV_THEME_H

#include "nativeviz/nv_types.h"
#include <vector>
#include <string>

namespace nv {

struct NVTheme {
    std::string name;

    // Background
    NVColor backgroundColor = {1, 1, 1, 1};
    NVColor plotAreaColor = {1, 1, 1, 1};

    // Series colors (cycled for multi-series)
    std::vector<NVColor> seriesColors;

    // Axis
    NVColor axisLineColor = {0.3f, 0.3f, 0.3f, 1};
    NVColor axisLabelColor = {0.2f, 0.2f, 0.2f, 1};
    NVColor gridLineColor = {0.9f, 0.9f, 0.9f, 1};
    float axisLineWidth = 1.0f;
    float gridLineWidth = 0.5f;

    // Text
    std::string fontFamily = "Arial";
    float titleFontSize = 16.0f;
    float labelFontSize = 11.0f;
    float tickFontSize = 10.0f;
    NVColor titleColor = {0.1f, 0.1f, 0.1f, 1};

    // Legend
    NVColor legendBorderColor = {0.85f, 0.85f, 0.85f, 1};
    NVColor legendBackgroundColor = {1, 1, 1, 0.95f};
    float legendFontSize = 11.0f;

    // Chart-specific
    float lineWidth = 2.0f;
    float barCornerRadius = 2.0f;
    float pointRadius = 4.0f;
    float barGap = 0.2f;  // fraction of bar width

    // Built-in themes
    static NVTheme excel();
    static NVTheme material();
    static NVTheme solarized();
    static NVTheme dark();
    static NVTheme monochrome();
    static NVTheme pastel();
};

} // namespace nv

#endif // NV_THEME_H
