#ifndef NV_RENDERER_2D_H
#define NV_RENDERER_2D_H

#include "nativeviz/nv_types.h"
#include <memory>
#include <vector>

namespace nv {

class NVDevice;
class NVCommandBuffer;
class NVBuffer;
class NVTexture;
class NVPipeline;

// GPU-side shape instance (must match sdf_shapes.metal ShapeInstance)
struct ShapeInstance {
    float position[2];
    float size[2];
    float fillColor[4];
    float strokeColor[4];
    float strokeWidth;
    float cornerRadius;
    float rotation;
    uint32_t shapeType;
    float param1;
    float param2;
    float param3;
    float _pad;
};
static_assert(sizeof(ShapeInstance) == 80, "ShapeInstance must be 80 bytes");

// GPU-side line vertex (must match line_strip.metal LineVertex)
struct LineVertex {
    float position[2];
    float normal[2];
    float side;
    float distance;
};

// High-level shape drawing parameters
struct ShapeParams {
    NVShapeType type = NV_SHAPE_RECTANGLE;
    NVRect bounds = {0, 0, 100, 100};
    NVColor fillColor = {1, 1, 1, 1};
    NVColor strokeColor = {0, 0, 0, 1};
    float strokeWidth = 0;
    float cornerRadius = 0;
    float rotation = 0;       // radians
    float param1 = 0;         // polygon sides / ring inner radius
    float param2 = 0;         // star inner ratio / arc start angle
    float param3 = 0;         // arc end angle
};

struct LineParams {
    NVColor color = {0, 0, 0, 1};
    float width = 2.0f;
    float dashLength = 0;     // 0 = solid
    float gapLength = 0;
    NVLineCap cap = NV_LINE_CAP_ROUND;
    NVLineJoin join = NV_LINE_JOIN_ROUND;
};

class NVRenderer2D {
public:
    explicit NVRenderer2D(NVDevice* device);
    ~NVRenderer2D();

    // Begin/end frame
    void beginFrame(NVTexture* target, float width, float height, float pixelRatio = 1.0f);
    void endFrame();

    // Shape drawing
    void drawShape(const ShapeParams& params);
    void drawRect(NVRect bounds, NVColor fill, float cornerRadius = 0);
    void drawCircle(float cx, float cy, float radius, NVColor fill);
    void drawEllipse(NVRect bounds, NVColor fill);
    void drawRing(float cx, float cy, float outerR, float innerR, NVColor fill);
    void drawPolygon(float cx, float cy, float radius, int sides, NVColor fill);
    void drawStar(float cx, float cy, float radius, int points, float innerRatio, NVColor fill);
    void drawArc(float cx, float cy, float radius, float startAngle, float endAngle, NVColor fill, float thickness = 0);

    // Line drawing
    void drawLine(NVPoint a, NVPoint b, const LineParams& params);
    void drawPolyline(const NVPoint* points, size_t count, const LineParams& params);

    // Text (placeholder — will be implemented with NVTextEngine)
    void drawText(const char* text, NVPoint position, NVColor color, float fontSize = 14.0f);

    // State
    void setClipRect(NVRect rect);
    void clearClipRect();

    // Flush batched draw calls
    void flush();

private:
    void flushShapes();
    void flushLines();
    void ensureShapeCapacity(size_t count);
    void generateLineGeometry(const NVPoint* points, size_t count, const LineParams& params);

    NVDevice* m_device;
    NVCommandBuffer* m_commandBuffer = nullptr;
    NVTexture* m_target = nullptr;

    // Shape batching
    std::vector<ShapeInstance> m_shapeBatch;
    std::unique_ptr<NVBuffer> m_shapeBuffer;
    std::unique_ptr<NVBuffer> m_uniformBuffer;
    std::unique_ptr<NVPipeline> m_shapePipeline;
    static constexpr size_t MAX_SHAPES_PER_BATCH = 65536;

    // Line batching
    std::vector<LineVertex> m_lineVertices;
    std::unique_ptr<NVBuffer> m_lineBuffer;
    std::unique_ptr<NVBuffer> m_lineUniformBuffer;
    std::unique_ptr<NVPipeline> m_linePipeline;
    LineParams m_currentLineParams;

    // Frame state
    float m_viewportWidth = 0;
    float m_viewportHeight = 0;
    float m_pixelRatio = 1.0f;
    bool m_inFrame = false;
};

} // namespace nv

#endif // NV_RENDERER_2D_H
