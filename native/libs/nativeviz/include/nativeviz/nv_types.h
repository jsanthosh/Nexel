#ifndef NV_TYPES_H
#define NV_TYPES_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// --- Enums ---

typedef enum NVBackend {
    NV_BACKEND_METAL = 0,
    NV_BACKEND_VULKAN = 1,
    NV_BACKEND_D3D12 = 2,
    NV_BACKEND_AUTO = 99
} NVBackend;

typedef enum NVPixelFormat {
    NV_PIXEL_FORMAT_RGBA8_UNORM = 0,
    NV_PIXEL_FORMAT_BGRA8_UNORM = 1,
    NV_PIXEL_FORMAT_R8_UNORM = 2,
    NV_PIXEL_FORMAT_RG16_FLOAT = 3,
    NV_PIXEL_FORMAT_RGBA16_FLOAT = 4,
    NV_PIXEL_FORMAT_RGBA32_FLOAT = 5,
    NV_PIXEL_FORMAT_DEPTH32_FLOAT = 6
} NVPixelFormat;

typedef enum NVBufferUsage {
    NV_BUFFER_USAGE_VERTEX = 0,
    NV_BUFFER_USAGE_INDEX = 1,
    NV_BUFFER_USAGE_UNIFORM = 2,
    NV_BUFFER_USAGE_STORAGE = 3
} NVBufferUsage;

typedef enum NVShaderStage {
    NV_SHADER_STAGE_VERTEX = 0,
    NV_SHADER_STAGE_FRAGMENT = 1,
    NV_SHADER_STAGE_COMPUTE = 2
} NVShaderStage;

typedef enum NVPrimitiveType {
    NV_PRIMITIVE_TRIANGLE = 0,
    NV_PRIMITIVE_TRIANGLE_STRIP = 1,
    NV_PRIMITIVE_LINE = 2,
    NV_PRIMITIVE_LINE_STRIP = 3,
    NV_PRIMITIVE_POINT = 4
} NVPrimitiveType;

typedef enum NVBlendMode {
    NV_BLEND_NONE = 0,
    NV_BLEND_ALPHA = 1,
    NV_BLEND_ADDITIVE = 2,
    NV_BLEND_PREMULTIPLIED_ALPHA = 3
} NVBlendMode;

typedef enum NVShapeType {
    NV_SHAPE_RECTANGLE = 0,
    NV_SHAPE_ROUNDED_RECT = 1,
    NV_SHAPE_CIRCLE = 2,
    NV_SHAPE_ELLIPSE = 3,
    NV_SHAPE_RING = 4,
    NV_SHAPE_POLYGON = 5,
    NV_SHAPE_STAR = 6,
    NV_SHAPE_ARC = 7,
    NV_SHAPE_LINE = 8
} NVShapeType;

typedef enum NVGradientType {
    NV_GRADIENT_LINEAR = 0,
    NV_GRADIENT_RADIAL = 1,
    NV_GRADIENT_CONIC = 2
} NVGradientType;

typedef enum NVChartType {
    NV_CHART_LINE = 0,
    NV_CHART_BAR = 1,
    NV_CHART_SCATTER = 2,
    NV_CHART_PIE = 3,
    NV_CHART_AREA = 4,
    NV_CHART_DONUT = 5,
    NV_CHART_HISTOGRAM = 6,
    NV_CHART_CANDLESTICK = 7,
    NV_CHART_WATERFALL = 8,
    NV_CHART_RADAR = 9,
    NV_CHART_TREEMAP = 10,
    NV_CHART_BUBBLE = 11,
    NV_CHART_FUNNEL = 12
} NVChartType;

typedef enum NVAxisType {
    NV_AXIS_LINEAR = 0,
    NV_AXIS_LOGARITHMIC = 1,
    NV_AXIS_CATEGORY = 2,
    NV_AXIS_DATETIME = 3
} NVAxisType;

typedef enum NVAxisPosition {
    NV_AXIS_BOTTOM = 0,
    NV_AXIS_LEFT = 1,
    NV_AXIS_TOP = 2,
    NV_AXIS_RIGHT = 3
} NVAxisPosition;

typedef enum NVLineJoin {
    NV_LINE_JOIN_MITER = 0,
    NV_LINE_JOIN_BEVEL = 1,
    NV_LINE_JOIN_ROUND = 2
} NVLineJoin;

typedef enum NVLineCap {
    NV_LINE_CAP_BUTT = 0,
    NV_LINE_CAP_ROUND = 1,
    NV_LINE_CAP_SQUARE = 2
} NVLineCap;

typedef enum NVMarkerShape {
    NV_MARKER_CIRCLE = 0,
    NV_MARKER_SQUARE = 1,
    NV_MARKER_DIAMOND = 2,
    NV_MARKER_TRIANGLE = 3,
    NV_MARKER_CROSS = 4,
    NV_MARKER_NONE = 99
} NVMarkerShape;

// --- Structs ---

typedef struct NVColor {
    float r, g, b, a;
} NVColor;

typedef struct NVRect {
    float x, y, width, height;
} NVRect;

typedef struct NVPoint {
    float x, y;
} NVPoint;

typedef struct NVSize {
    float width, height;
} NVSize;

typedef struct NVMatrix3x3 {
    float m[9]; // column-major 3x3 affine transform
} NVMatrix3x3;

typedef struct NVGradientStop {
    float position; // 0.0 - 1.0
    NVColor color;
} NVGradientStop;

typedef struct NVViewport {
    float x, y, width, height;
} NVViewport;

// --- Inline helpers (C++) ---

#ifdef __cplusplus
} // extern "C"

namespace nv {

inline NVColor color(float r, float g, float b, float a = 1.0f) {
    return {r, g, b, a};
}

inline NVColor colorHex(uint32_t hex) {
    return {
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        1.0f
    };
}

inline NVRect rect(float x, float y, float w, float h) {
    return {x, y, w, h};
}

inline NVPoint point(float x, float y) {
    return {x, y};
}

inline NVMatrix3x3 identity() {
    return {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
}

} // namespace nv
#endif

#endif // NV_TYPES_H
