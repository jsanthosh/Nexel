#include <metal_stdlib>
using namespace metal;

struct LineUniforms {
    float4x4 projectionMatrix;
    float2 viewportSize;
    float lineWidth;
    float pixelRatio;
    float4 lineColor;
    float dashLength;   // 0 = solid
    float gapLength;
};

struct LineVertex {
    float2 position;    // point on the line
    float2 normal;      // perpendicular direction
    float side;         // -1 or +1 (which side of the line)
    float distance;     // cumulative distance along the line
};

struct LineOut {
    float4 position [[position]];
    float4 color;
    float distance;
    float dashLength;
    float gapLength;
    float edgeDist;     // distance from line center (for anti-aliasing)
    float halfWidth;
};

// Vertex shader for anti-aliased line strip
// Each line segment generates a quad (4 vertices, 2 triangles)
vertex LineOut line_strip_vertex(
    uint vertexID [[vertex_id]],
    constant LineUniforms& uniforms [[buffer(0)]],
    constant LineVertex* vertices [[buffer(1)]]
) {
    LineVertex v = vertices[vertexID];

    // Expand vertex position perpendicular to the line
    float halfWidth = (uniforms.lineWidth * 0.5 + uniforms.pixelRatio); // extra pixel for AA
    float2 worldPos = v.position + v.normal * v.side * halfWidth;

    // Convert to NDC
    float2 ndc = worldPos / uniforms.viewportSize * 2.0 - 1.0;
    ndc.y = -ndc.y;

    LineOut out;
    out.position = float4(ndc, 0, 1);
    out.color = uniforms.lineColor;
    out.distance = v.distance;
    out.dashLength = uniforms.dashLength;
    out.gapLength = uniforms.gapLength;
    out.edgeDist = v.side * halfWidth;
    out.halfWidth = uniforms.lineWidth * 0.5;
    return out;
}

fragment float4 line_strip_fragment(LineOut in [[stage_in]]) {
    // Anti-aliased edge
    float d = abs(in.edgeDist) - in.halfWidth;
    float alpha = 1.0 - smoothstep(-1.0, 1.0, d);

    // Dash pattern
    if (in.dashLength > 0.0) {
        float pattern = fmod(in.distance, in.dashLength + in.gapLength);
        if (pattern > in.dashLength) {
            alpha = 0.0;
        }
    }

    return float4(in.color.rgb, in.color.a * alpha);
}
