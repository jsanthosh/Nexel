#include "NVAxis.h"
#include "../render/NVRenderer2D.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace nv {

NVAxis::NVAxis() = default;

void NVAxis::configure(const NVAxisConfig& config) {
    m_config = config;
}

void NVAxis::setDataRange(double min, double max) {
    if (m_config.autoRange) {
        // Add 5% padding
        double range = max - min;
        if (range < 1e-10) range = 1.0;
        m_effectiveMin = min - range * 0.05;
        m_effectiveMax = max + range * 0.05;

        // For bar charts, start at 0 if data is all positive
        if (min >= 0) m_effectiveMin = 0;
    } else {
        m_effectiveMin = m_config.minValue;
        m_effectiveMax = m_config.maxValue;
    }
}

void NVAxis::layout(float axisLength, float perpOffset) {
    m_axisLength = axisLength;
    m_perpOffset = perpOffset;

    switch (m_config.type) {
        case NV_AXIS_CATEGORY:
            generateCategoryTicks();
            break;
        case NV_AXIS_LOGARITHMIC:
            generateLogTicks();
            break;
        default:
            generateNiceTicks();
            break;
    }
}

float NVAxis::dataToPixel(double value) const {
    double range = m_effectiveMax - m_effectiveMin;
    if (range < 1e-10) return 0;
    float t = static_cast<float>((value - m_effectiveMin) / range);

    // Y axis is inverted (0 at bottom)
    if (m_config.position == NV_AXIS_LEFT || m_config.position == NV_AXIS_RIGHT) {
        return m_axisLength * (1.0f - t);
    }
    return m_axisLength * t;
}

double NVAxis::pixelToData(float pixel) const {
    double range = m_effectiveMax - m_effectiveMin;
    float t;
    if (m_config.position == NV_AXIS_LEFT || m_config.position == NV_AXIS_RIGHT) {
        t = 1.0f - pixel / m_axisLength;
    } else {
        t = pixel / m_axisLength;
    }
    return m_effectiveMin + range * t;
}

void NVAxis::render(NVRenderer2D& renderer, const NVTheme& theme, NVRect plotArea) const {
    bool isHorizontal = (m_config.position == NV_AXIS_BOTTOM || m_config.position == NV_AXIS_TOP);
    LineParams axisLine;
    axisLine.color = theme.axisLineColor;
    axisLine.width = theme.axisLineWidth;

    // Draw axis line
    if (isHorizontal) {
        float y = (m_config.position == NV_AXIS_BOTTOM) ? plotArea.y + plotArea.height : plotArea.y;
        renderer.drawLine({plotArea.x, y}, {plotArea.x + plotArea.width, y}, axisLine);
    } else {
        float x = (m_config.position == NV_AXIS_LEFT) ? plotArea.x : plotArea.x + plotArea.width;
        renderer.drawLine({x, plotArea.y}, {x, plotArea.y + plotArea.height}, axisLine);
    }

    // Draw ticks and grid lines
    LineParams gridLine;
    gridLine.color = theme.gridLineColor;
    gridLine.width = theme.gridLineWidth;

    for (const auto& tick : m_ticks) {
        if (isHorizontal) {
            float x = plotArea.x + tick.position;
            float y = (m_config.position == NV_AXIS_BOTTOM)
                ? plotArea.y + plotArea.height
                : plotArea.y;

            // Tick mark
            float tickLen = 5;
            float dir = (m_config.position == NV_AXIS_BOTTOM) ? 1.0f : -1.0f;
            renderer.drawLine({x, y}, {x, y + tickLen * dir}, axisLine);

            // Grid line
            if (m_config.showGrid) {
                renderer.drawLine({x, plotArea.y}, {x, plotArea.y + plotArea.height}, gridLine);
            }

            // Label
            if (m_config.showLabels) {
                float labelY = y + (tickLen + 12) * dir;
                renderer.drawText(tick.label.c_str(), {x, labelY}, theme.axisLabelColor, theme.tickFontSize);
            }
        } else {
            float y = plotArea.y + tick.position;
            float x = (m_config.position == NV_AXIS_LEFT)
                ? plotArea.x
                : plotArea.x + plotArea.width;

            // Tick mark
            float tickLen = 5;
            float dir = (m_config.position == NV_AXIS_LEFT) ? -1.0f : 1.0f;
            renderer.drawLine({x, y}, {x + tickLen * dir, y}, axisLine);

            // Grid line
            if (m_config.showGrid) {
                renderer.drawLine({plotArea.x, y}, {plotArea.x + plotArea.width, y}, gridLine);
            }

            // Label
            if (m_config.showLabels) {
                float labelX = x + (tickLen + 5) * dir;
                renderer.drawText(tick.label.c_str(), {labelX, y}, theme.axisLabelColor, theme.tickFontSize);
            }
        }
    }
}

void NVAxis::generateNiceTicks() {
    m_ticks.clear();
    double range = m_effectiveMax - m_effectiveMin;
    if (range < 1e-10) return;

    // Wilkinson-like "nice numbers" algorithm
    double rawStep = range / m_config.desiredTickCount;
    m_tickStep = niceNumber(rawStep, true);

    double tickMin = std::ceil(m_effectiveMin / m_tickStep) * m_tickStep;

    for (double v = tickMin; v <= m_effectiveMax + m_tickStep * 0.01; v += m_tickStep) {
        NVTick tick;
        tick.value = v;
        tick.label = formatValue(v);
        tick.position = dataToPixel(v);
        m_ticks.push_back(tick);
    }
}

void NVAxis::generateCategoryTicks() {
    m_ticks.clear();
    size_t n = m_config.categories.size();
    if (n == 0) return;

    m_effectiveMin = -0.5;
    m_effectiveMax = static_cast<double>(n) - 0.5;

    for (size_t i = 0; i < n; i++) {
        NVTick tick;
        tick.value = static_cast<double>(i);
        tick.label = m_config.categories[i];
        tick.position = dataToPixel(static_cast<double>(i));
        m_ticks.push_back(tick);
    }
}

void NVAxis::generateLogTicks() {
    m_ticks.clear();
    double logMin = std::log10(std::max(m_effectiveMin, 1e-10));
    double logMax = std::log10(std::max(m_effectiveMax, 1e-10));

    int startPow = static_cast<int>(std::floor(logMin));
    int endPow = static_cast<int>(std::ceil(logMax));

    for (int p = startPow; p <= endPow; p++) {
        double v = std::pow(10.0, p);
        if (v >= m_effectiveMin && v <= m_effectiveMax) {
            NVTick tick;
            tick.value = v;
            tick.label = formatValue(v);
            // Log scale: pixel position is linear in log space
            float t = static_cast<float>((std::log10(v) - logMin) / (logMax - logMin));
            tick.position = m_axisLength * t;
            m_ticks.push_back(tick);
        }
    }
}

std::string NVAxis::formatValue(double value) const {
    std::ostringstream ss;

    if (!m_config.prefix.empty()) ss << m_config.prefix;

    if (m_config.useKMB && std::abs(value) >= 1e9) {
        ss << std::fixed << std::setprecision(1) << (value / 1e9) << "B";
    } else if (m_config.useKMB && std::abs(value) >= 1e6) {
        ss << std::fixed << std::setprecision(1) << (value / 1e6) << "M";
    } else if (m_config.useKMB && std::abs(value) >= 1e3) {
        ss << std::fixed << std::setprecision(1) << (value / 1e3) << "K";
    } else {
        int dp = m_config.decimalPlaces;
        if (dp < 0) {
            // Auto: use minimal precision
            if (std::abs(value - std::round(value)) < 1e-9) {
                dp = 0;
            } else if (std::abs(value * 10 - std::round(value * 10)) < 1e-9) {
                dp = 1;
            } else {
                dp = 2;
            }
        }
        ss << std::fixed << std::setprecision(dp) << value;
    }

    if (!m_config.suffix.empty()) ss << m_config.suffix;

    return ss.str();
}

double NVAxis::niceNumber(double value, bool round) {
    double exp = std::floor(std::log10(std::abs(value)));
    double fraction = value / std::pow(10.0, exp);
    double nice;

    if (round) {
        if (fraction < 1.5) nice = 1;
        else if (fraction < 3) nice = 2;
        else if (fraction < 7) nice = 5;
        else nice = 10;
    } else {
        if (fraction <= 1) nice = 1;
        else if (fraction <= 2) nice = 2;
        else if (fraction <= 5) nice = 5;
        else nice = 10;
    }

    return nice * std::pow(10.0, exp);
}

} // namespace nv
