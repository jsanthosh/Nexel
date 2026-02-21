#include "NVRenderer2D.h"
#include "../hal/NVDevice.h"
#include "../hal/NVCommandBuffer.h"
#include "../hal/NVBuffer.h"
#include "../hal/NVTexture.h"
#include "../hal/NVPipeline.h"
#include <cmath>
#include <cstring>

namespace nv {

// Uniform buffer layout (must match sdf_shapes.metal NVUniforms)
struct NVUniforms {
    float projectionMatrix[16]; // 4x4 identity (using 2D NDC directly)
    float viewportSize[2];
    float time;
    float pixelRatio;
};

// Line uniform buffer layout (must match line_strip.metal LineUniforms)
struct LineUniforms {
    float projectionMatrix[16];
    float viewportSize[2];
    float lineWidth;
    float pixelRatio;
    float lineColor[4];
    float dashLength;
    float gapLength;
};

NVRenderer2D::NVRenderer2D(NVDevice* device) : m_device(device) {
    m_shapeBatch.reserve(4096);
    m_lineVertices.reserve(8192);

    // Create GPU buffers
    m_shapeBuffer = device->createBuffer(MAX_SHAPES_PER_BATCH * sizeof(ShapeInstance), NV_BUFFER_USAGE_VERTEX);
    m_uniformBuffer = device->createBuffer(sizeof(NVUniforms), NV_BUFFER_USAGE_UNIFORM);
    m_lineBuffer = device->createBuffer(65536 * sizeof(LineVertex), NV_BUFFER_USAGE_VERTEX);
    m_lineUniformBuffer = device->createBuffer(sizeof(LineUniforms), NV_BUFFER_USAGE_UNIFORM);

    // Create pipelines
    NVPipelineDesc shapeDesc;
    shapeDesc.vertexFunction = "sdf_shape_vertex";
    shapeDesc.fragmentFunction = "sdf_shape_fragment";
    shapeDesc.blendMode = NV_BLEND_PREMULTIPLIED_ALPHA;
    m_shapePipeline = device->createRenderPipeline(shapeDesc);

    NVPipelineDesc lineDesc;
    lineDesc.vertexFunction = "line_strip_vertex";
    lineDesc.fragmentFunction = "line_strip_fragment";
    lineDesc.blendMode = NV_BLEND_ALPHA;
    lineDesc.primitiveType = NV_PRIMITIVE_TRIANGLE_STRIP;
    m_linePipeline = device->createRenderPipeline(lineDesc);
}

NVRenderer2D::~NVRenderer2D() = default;

void NVRenderer2D::beginFrame(NVTexture* target, float width, float height, float pixelRatio) {
    m_target = target;
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_pixelRatio = pixelRatio;
    m_inFrame = true;

    // Update uniform buffer
    NVUniforms uniforms{};
    // Identity projection (we do NDC conversion in vertex shader)
    uniforms.projectionMatrix[0] = 1;
    uniforms.projectionMatrix[5] = 1;
    uniforms.projectionMatrix[10] = 1;
    uniforms.projectionMatrix[15] = 1;
    uniforms.viewportSize[0] = width;
    uniforms.viewportSize[1] = height;
    uniforms.time = 0;
    uniforms.pixelRatio = pixelRatio;
    m_uniformBuffer->upload(&uniforms, sizeof(uniforms));

    m_shapeBatch.clear();
    m_lineVertices.clear();
}

void NVRenderer2D::endFrame() {
    flush();
    m_inFrame = false;
    m_target = nullptr;
}

void NVRenderer2D::drawShape(const ShapeParams& params) {
    if (m_shapeBatch.size() >= MAX_SHAPES_PER_BATCH) {
        flushShapes();
    }

    ShapeInstance inst{};
    inst.position[0] = params.bounds.x + params.bounds.width * 0.5f;
    inst.position[1] = params.bounds.y + params.bounds.height * 0.5f;
    inst.size[0] = params.bounds.width;
    inst.size[1] = params.bounds.height;
    inst.fillColor[0] = params.fillColor.r;
    inst.fillColor[1] = params.fillColor.g;
    inst.fillColor[2] = params.fillColor.b;
    inst.fillColor[3] = params.fillColor.a;
    inst.strokeColor[0] = params.strokeColor.r;
    inst.strokeColor[1] = params.strokeColor.g;
    inst.strokeColor[2] = params.strokeColor.b;
    inst.strokeColor[3] = params.strokeColor.a;
    inst.strokeWidth = params.strokeWidth;
    inst.cornerRadius = params.cornerRadius;
    inst.rotation = params.rotation;
    inst.shapeType = static_cast<uint32_t>(params.type);
    inst.param1 = params.param1;
    inst.param2 = params.param2;
    inst.param3 = params.param3;
    inst._pad = 0;

    m_shapeBatch.push_back(inst);
}

void NVRenderer2D::drawRect(NVRect bounds, NVColor fill, float cornerRadius) {
    ShapeParams p;
    p.type = cornerRadius > 0 ? NV_SHAPE_ROUNDED_RECT : NV_SHAPE_RECTANGLE;
    p.bounds = bounds;
    p.fillColor = fill;
    p.cornerRadius = cornerRadius;
    drawShape(p);
}

void NVRenderer2D::drawCircle(float cx, float cy, float radius, NVColor fill) {
    ShapeParams p;
    p.type = NV_SHAPE_CIRCLE;
    p.bounds = {cx - radius, cy - radius, radius * 2, radius * 2};
    p.fillColor = fill;
    drawShape(p);
}

void NVRenderer2D::drawEllipse(NVRect bounds, NVColor fill) {
    ShapeParams p;
    p.type = NV_SHAPE_ELLIPSE;
    p.bounds = bounds;
    p.fillColor = fill;
    drawShape(p);
}

void NVRenderer2D::drawRing(float cx, float cy, float outerR, float innerR, NVColor fill) {
    ShapeParams p;
    p.type = NV_SHAPE_RING;
    p.bounds = {cx - outerR, cy - outerR, outerR * 2, outerR * 2};
    p.fillColor = fill;
    p.param1 = innerR;
    drawShape(p);
}

void NVRenderer2D::drawPolygon(float cx, float cy, float radius, int sides, NVColor fill) {
    ShapeParams p;
    p.type = NV_SHAPE_POLYGON;
    p.bounds = {cx - radius, cy - radius, radius * 2, radius * 2};
    p.fillColor = fill;
    p.param1 = static_cast<float>(sides);
    drawShape(p);
}

void NVRenderer2D::drawStar(float cx, float cy, float radius, int points, float innerRatio, NVColor fill) {
    ShapeParams p;
    p.type = NV_SHAPE_STAR;
    p.bounds = {cx - radius, cy - radius, radius * 2, radius * 2};
    p.fillColor = fill;
    p.param1 = static_cast<float>(points);
    p.param2 = innerRatio;
    drawShape(p);
}

void NVRenderer2D::drawArc(float cx, float cy, float radius, float startAngle, float endAngle, NVColor fill, float thickness) {
    ShapeParams p;
    p.type = NV_SHAPE_ARC;
    p.bounds = {cx - radius, cy - radius, radius * 2, radius * 2};
    p.fillColor = fill;
    p.strokeWidth = thickness > 0 ? thickness : 2.0f;
    p.param2 = startAngle;
    p.param3 = endAngle;
    drawShape(p);
}

void NVRenderer2D::drawLine(NVPoint a, NVPoint b, const LineParams& params) {
    NVPoint points[2] = {a, b};
    drawPolyline(points, 2, params);
}

void NVRenderer2D::drawPolyline(const NVPoint* points, size_t count, const LineParams& params) {
    if (count < 2) return;
    // If line params changed, flush previous batch
    if (!m_lineVertices.empty()) {
        // Simple check: flush if params differ
        flushLines();
    }
    m_currentLineParams = params;
    generateLineGeometry(points, count, params);
}

void NVRenderer2D::generateLineGeometry(const NVPoint* points, size_t count, const LineParams& /*params*/) {
    float cumulativeDist = 0;

    for (size_t i = 0; i < count - 1; i++) {
        NVPoint a = points[i];
        NVPoint b = points[i + 1];

        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) continue;

        // Normal perpendicular to line direction
        float nx = -dy / len;
        float ny = dx / len;

        float distA = cumulativeDist;
        float distB = cumulativeDist + len;
        cumulativeDist = distB;

        // Generate 4 vertices for this segment (triangle strip)
        LineVertex v0{};
        v0.position[0] = a.x; v0.position[1] = a.y;
        v0.normal[0] = nx; v0.normal[1] = ny;
        v0.side = -1.0f; v0.distance = distA;

        LineVertex v1{};
        v1.position[0] = a.x; v1.position[1] = a.y;
        v1.normal[0] = nx; v1.normal[1] = ny;
        v1.side = 1.0f; v1.distance = distA;

        LineVertex v2{};
        v2.position[0] = b.x; v2.position[1] = b.y;
        v2.normal[0] = nx; v2.normal[1] = ny;
        v2.side = -1.0f; v2.distance = distB;

        LineVertex v3{};
        v3.position[0] = b.x; v3.position[1] = b.y;
        v3.normal[0] = nx; v3.normal[1] = ny;
        v3.side = 1.0f; v3.distance = distB;

        m_lineVertices.push_back(v0);
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
        m_lineVertices.push_back(v3);

        // Degenerate triangles to connect segments (if not last segment)
        if (i < count - 2) {
            m_lineVertices.push_back(v3);  // repeat last
            // Next segment's first will be added in next iteration
            // Add degenerate connection
            NVPoint c = points[i + 1];
            LineVertex degen{};
            degen.position[0] = c.x; degen.position[1] = c.y;
            degen.normal[0] = nx; degen.normal[1] = ny;
            degen.side = -1.0f; degen.distance = distB;
            m_lineVertices.push_back(degen);
        }
    }
}

void NVRenderer2D::drawText(const char* /*text*/, NVPoint /*position*/, NVColor /*color*/, float /*fontSize*/) {
    // Placeholder — will be implemented with NVTextEngine
}

void NVRenderer2D::setClipRect(NVRect /*rect*/) {
    // TODO: implement scissor rect
}

void NVRenderer2D::clearClipRect() {
    // TODO: clear scissor rect
}

void NVRenderer2D::flush() {
    flushShapes();
    flushLines();
}

void NVRenderer2D::flushShapes() {
    if (m_shapeBatch.empty() || !m_target || !m_shapePipeline) return;

    // Upload instance data
    size_t dataSize = m_shapeBatch.size() * sizeof(ShapeInstance);
    m_shapeBuffer->upload(m_shapeBatch.data(), dataSize);

    // Create command buffer and render
    auto cmd = m_device->createCommandBuffer();
    cmd->beginRenderPass(m_target, {0, 0, 0, 0}); // transparent clear

    cmd->setRenderPipeline(m_shapePipeline.get());
    cmd->setViewport({0, 0, m_viewportWidth, m_viewportHeight});
    cmd->setVertexBuffer(m_uniformBuffer.get(), 0);
    cmd->setVertexBuffer(m_shapeBuffer.get(), 1);

    // Draw instanced quads (4 vertices per quad, N instances)
    cmd->draw(NV_PRIMITIVE_TRIANGLE_STRIP, 4, static_cast<uint32_t>(m_shapeBatch.size()));

    cmd->endRenderPass();
    cmd->commit();

    m_shapeBatch.clear();
}

void NVRenderer2D::flushLines() {
    if (m_lineVertices.empty() || !m_target || !m_linePipeline) return;

    // Upload line uniforms
    LineUniforms lu{};
    lu.projectionMatrix[0] = 1; lu.projectionMatrix[5] = 1;
    lu.projectionMatrix[10] = 1; lu.projectionMatrix[15] = 1;
    lu.viewportSize[0] = m_viewportWidth;
    lu.viewportSize[1] = m_viewportHeight;
    lu.lineWidth = m_currentLineParams.width;
    lu.pixelRatio = m_pixelRatio;
    lu.lineColor[0] = m_currentLineParams.color.r;
    lu.lineColor[1] = m_currentLineParams.color.g;
    lu.lineColor[2] = m_currentLineParams.color.b;
    lu.lineColor[3] = m_currentLineParams.color.a;
    lu.dashLength = m_currentLineParams.dashLength;
    lu.gapLength = m_currentLineParams.gapLength;
    m_lineUniformBuffer->upload(&lu, sizeof(lu));

    // Upload vertex data
    size_t dataSize = m_lineVertices.size() * sizeof(LineVertex);
    m_lineBuffer->upload(m_lineVertices.data(), dataSize);

    auto cmd = m_device->createCommandBuffer();
    cmd->beginRenderPass(m_target, {0, 0, 0, 0});

    cmd->setRenderPipeline(m_linePipeline.get());
    cmd->setViewport({0, 0, m_viewportWidth, m_viewportHeight});
    cmd->setVertexBuffer(m_lineUniformBuffer.get(), 0);
    cmd->setVertexBuffer(m_lineBuffer.get(), 1);

    cmd->draw(NV_PRIMITIVE_TRIANGLE_STRIP, static_cast<uint32_t>(m_lineVertices.size()));

    cmd->endRenderPass();
    cmd->commit();

    m_lineVertices.clear();
}

void NVRenderer2D::ensureShapeCapacity(size_t count) {
    if (m_shapeBatch.size() + count > MAX_SHAPES_PER_BATCH) {
        flushShapes();
    }
}

} // namespace nv
