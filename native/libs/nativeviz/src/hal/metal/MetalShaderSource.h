#ifndef NV_METAL_SHADER_SOURCE_H
#define NV_METAL_SHADER_SOURCE_H

#include <string_view>

namespace nv {
namespace shaders {

constexpr std::string_view kAllShaders = R"METAL(
// =============================================================================
// sdf_shapes.metal - SDF shape rendering (vertex + fragment)
// =============================================================================

#include <metal_stdlib>
using namespace metal;

// --- Shared types between CPU and GPU ---

struct NVUniforms {
    float4x4 projectionMatrix;
    float2 viewportSize;
    float time;
    float pixelRatio;
};

struct ShapeInstance {
    float2 position;    // center of shape in screen coords
    float2 size;        // width, height
    float4 fillColor;
    float4 strokeColor;
    float strokeWidth;
    float cornerRadius;
    float rotation;     // radians
    uint shapeType;     // NVShapeType enum
    float param1;       // extra param (sides for polygon, inner radius for ring, etc.)
    float param2;       // extra param (start angle for arc, etc.)
    float param3;       // extra param (end angle for arc, etc.)
    float _pad;
};

struct ShapeVertex {
    float4 position [[position]];
    float2 uv;
    float2 shapeSize;
    float4 fillColor;
    float4 strokeColor;
    float strokeWidth;
    float cornerRadius;
    uint shapeType;
    float param1;
    float param2;
    float param3;
    float pixelWidth;
};

// --- SDF functions ---

// Signed distance to a box with optional rounded corners
float sdRoundedBox(float2 p, float2 b, float r) {
    float2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// Signed distance to a circle
float sdCircle(float2 p, float r) {
    return length(p) - r;
}

// Signed distance to an ellipse (approximation)
float sdEllipse(float2 p, float2 r) {
    float2 q = p / r;
    float d = length(q) - 1.0;
    return d * min(r.x, r.y);
}

// Signed distance to a ring (donut)
float sdRing(float2 p, float outerR, float innerR) {
    float d = length(p);
    return max(d - outerR, -(d - innerR));
}

// Signed distance to a regular polygon
float sdPolygon(float2 p, float r, float n) {
    float an = M_PI_F / n;
    float he = r * cos(an);
    float2 bn = float2(cos(an), sin(an));

    float a = atan2(p.y, p.x);
    float ia = floor(a / (2.0 * an) + 0.5);
    a = a - 2.0 * an * ia;
    float2 q = float2(cos(a), abs(sin(a)));
    return dot(q, bn) - he;
}

// Signed distance to a star
float sdStar(float2 p, float outerR, float n, float innerRatio) {
    float an = M_PI_F / n;
    float en = M_PI_F / n;
    float2 acs = float2(cos(an), sin(an));
    float2 ecs = float2(cos(en), sin(en));

    float a = atan2(p.y, p.x);
    float ia = floor(a / (2.0 * an) + 0.5);
    a = a - 2.0 * an * ia;
    float2 q = float2(cos(a), abs(sin(a)));

    float innerR = outerR * innerRatio;
    float d = min(dot(q, acs) - outerR * cos(an),
                  length(q - acs * outerR));
    d = min(d, length(q * outerR - float2(innerR, 0)));
    return d;
}

// Signed distance to an arc segment
float sdArc(float2 p, float r, float startAngle, float endAngle) {
    float midAngle = (startAngle + endAngle) * 0.5;
    float halfArc = (endAngle - startAngle) * 0.5;

    // Rotate point to align arc center with +x axis
    float cs = cos(-midAngle);
    float sn = sin(-midAngle);
    float2 q = float2(p.x * cs - p.y * sn, p.x * sn + p.y * cs);

    float a = atan2(q.y, q.x);
    if (abs(a) < halfArc) {
        return abs(length(q) - r);
    }
    float2 tip1 = r * float2(cos(halfArc), sin(halfArc));
    float2 tip2 = r * float2(cos(halfArc), -sin(halfArc));
    return min(length(q - tip1), length(q - tip2));
}

// --- Vertex shader ---

vertex ShapeVertex sdf_shape_vertex(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant NVUniforms& uniforms [[buffer(0)]],
    constant ShapeInstance* instances [[buffer(1)]]
) {
    // Generate quad vertices: 0=TL, 1=TR, 2=BL, 3=BR (triangle strip)
    float2 quadPos[4] = {
        float2(-1, -1), float2(1, -1),
        float2(-1,  1), float2(1,  1)
    };
    float2 quadUV[4] = {
        float2(-1, -1), float2(1, -1),
        float2(-1,  1), float2(1,  1)
    };

    ShapeInstance inst = instances[instanceID];
    float2 pos = quadPos[vertexID];

    // Expand quad by stroke width + 2px for anti-aliasing
    float expand = inst.strokeWidth + 2.0 * uniforms.pixelRatio;
    float2 halfSize = inst.size * 0.5 + expand;

    // Apply rotation
    float cs = cos(inst.rotation);
    float sn = sin(inst.rotation);
    float2 rotated = float2(pos.x * cs - pos.y * sn, pos.x * sn + pos.y * cs);

    float2 worldPos = inst.position + rotated * halfSize;

    // Convert to NDC
    float2 ndc = worldPos / uniforms.viewportSize * 2.0 - 1.0;
    ndc.y = -ndc.y; // Flip Y for Metal

    ShapeVertex out;
    out.position = float4(ndc, 0, 1);
    out.uv = quadUV[vertexID] * halfSize;
    out.shapeSize = inst.size;
    out.fillColor = inst.fillColor;
    out.strokeColor = inst.strokeColor;
    out.strokeWidth = inst.strokeWidth;
    out.cornerRadius = inst.cornerRadius;
    out.shapeType = inst.shapeType;
    out.param1 = inst.param1;
    out.param2 = inst.param2;
    out.param3 = inst.param3;
    out.pixelWidth = uniforms.pixelRatio;
    return out;
}

// --- Fragment shader ---

fragment float4 sdf_shape_fragment(ShapeVertex in [[stage_in]]) {
    float2 p = in.uv;
    float2 halfSize = in.shapeSize * 0.5;
    float pw = in.pixelWidth;
    float dist;

    switch (in.shapeType) {
        case 0: // Rectangle
            dist = sdRoundedBox(p, halfSize, 0.0);
            break;
        case 1: // Rounded rect
            dist = sdRoundedBox(p, halfSize, in.cornerRadius);
            break;
        case 2: // Circle
            dist = sdCircle(p, min(halfSize.x, halfSize.y));
            break;
        case 3: // Ellipse
            dist = sdEllipse(p, halfSize);
            break;
        case 4: // Ring
            dist = sdRing(p, min(halfSize.x, halfSize.y), in.param1);
            break;
        case 5: // Polygon
            dist = sdPolygon(p, min(halfSize.x, halfSize.y), max(in.param1, 3.0));
            break;
        case 6: // Star
            dist = sdStar(p, min(halfSize.x, halfSize.y), max(in.param1, 3.0), in.param2);
            break;
        case 7: // Arc
            dist = sdArc(p, min(halfSize.x, halfSize.y), in.param2, in.param3);
            break;
        default: // Line (handled separately) or unknown
            dist = sdRoundedBox(p, halfSize, 0.0);
            break;
    }

    // Anti-aliased fill
    float fillAlpha = 1.0 - smoothstep(-pw, pw, dist);
    float4 color = in.fillColor * fillAlpha;

    // Stroke
    if (in.strokeWidth > 0.0) {
        float strokeDist = abs(dist) - in.strokeWidth * 0.5;
        float strokeAlpha = 1.0 - smoothstep(-pw, pw, strokeDist);
        color = mix(color, float4(in.strokeColor.rgb, in.strokeColor.a * strokeAlpha), strokeAlpha);
    }

    return color;
}

// =============================================================================
// line_strip.metal - Anti-aliased line strip rendering
// =============================================================================

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

// =============================================================================
// gradient.metal - Gradient texture generation (compute)
// =============================================================================

struct GradientStop {
    float position;
    float4 color;
};

struct GradientUniforms {
    uint type;          // 0=linear, 1=radial, 2=conic
    uint stopCount;
    float2 start;       // linear: start point; radial: center
    float2 end;         // linear: end point; radial: unused
    float radius;       // radial: outer radius
    float angle;        // conic: start angle
};

// Compute shader to generate a gradient texture
kernel void generate_gradient(
    texture2d<float, access::write> output [[texture(0)]],
    constant GradientUniforms& uniforms [[buffer(0)]],
    constant GradientStop* stops [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint width = output.get_width();
    uint height = output.get_height();
    if (gid.x >= width || gid.y >= height) return;

    float2 uv = float2(gid) / float2(width - 1, height - 1);
    float t = 0.0;

    switch (uniforms.type) {
        case 0: { // Linear
            float2 dir = uniforms.end - uniforms.start;
            float len = length(dir);
            if (len > 0.0) {
                float2 n = dir / len;
                t = dot(uv - uniforms.start, n) / len;
            }
            break;
        }
        case 1: { // Radial
            float d = length(uv - uniforms.start);
            t = d / max(uniforms.radius, 0.001);
            break;
        }
        case 2: { // Conic
            float2 d = uv - uniforms.start;
            float a = atan2(d.y, d.x) - uniforms.angle;
            t = fmod(a / (2.0 * M_PI_F) + 1.0, 1.0);
            break;
        }
    }

    t = clamp(t, 0.0, 1.0);

    // Interpolate between stops
    float4 color = stops[0].color;
    for (uint i = 1; i < uniforms.stopCount; i++) {
        if (t <= stops[i].position) {
            float range = stops[i].position - stops[i-1].position;
            float localT = (range > 0.0) ? (t - stops[i-1].position) / range : 0.0;
            color = mix(stops[i-1].color, stops[i].color, localT);
            break;
        }
        color = stops[i].color;
    }

    output.write(color, gid);
}

// =============================================================================
// blur.metal - Two-pass Gaussian blur (compute)
// =============================================================================

struct BlurUniforms {
    float2 texelSize;   // 1.0 / textureSize
    float radius;       // blur radius in pixels
    uint direction;     // 0 = horizontal, 1 = vertical
};

// Two-pass Gaussian blur (horizontal then vertical)
kernel void gaussian_blur(
    texture2d<float, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],
    constant BlurUniforms& uniforms [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint width = input.get_width();
    uint height = input.get_height();
    if (gid.x >= width || gid.y >= height) return;

    int radius = int(uniforms.radius);
    float sigma = uniforms.radius * 0.5;
    float sigma2 = 2.0 * sigma * sigma;

    float4 sum = float4(0);
    float weightSum = 0.0;

    for (int i = -radius; i <= radius; i++) {
        int2 offset = uniforms.direction == 0 ? int2(i, 0) : int2(0, i);
        int2 coord = int2(gid) + offset;

        // Clamp to texture bounds
        coord = clamp(coord, int2(0), int2(width - 1, height - 1));

        float weight = exp(-float(i * i) / sigma2);
        sum += input.read(uint2(coord)) * weight;
        weightSum += weight;
    }

    output.write(sum / weightSum, gid);
}

// =============================================================================
// decimation.metal - Data decimation for large datasets (compute)
// =============================================================================

struct DecimationUniforms {
    uint inputCount;     // number of input points
    uint outputCount;    // target number of output points (buckets)
    uint algorithm;      // 0 = min-max, 1 = LTTB
};

// Min-Max decimation: find min and max Y value per bucket
kernel void decimate_minmax(
    constant float* inputX [[buffer(0)]],
    constant float* inputY [[buffer(1)]],
    device float* outputX [[buffer(2)]],
    device float* outputY [[buffer(3)]],
    device uint* outputIndices [[buffer(4)]],
    constant DecimationUniforms& uniforms [[buffer(5)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= uniforms.outputCount) return;

    uint bucketSize = uniforms.inputCount / uniforms.outputCount;
    uint start = gid * bucketSize;
    uint end = min(start + bucketSize, uniforms.inputCount);

    // Each output bucket produces 2 points (min and max)
    uint outIdx = gid * 2;

    float minY = inputY[start];
    float maxY = inputY[start];
    uint minIdx = start;
    uint maxIdx = start;

    for (uint i = start + 1; i < end; i++) {
        if (inputY[i] < minY) {
            minY = inputY[i];
            minIdx = i;
        }
        if (inputY[i] > maxY) {
            maxY = inputY[i];
            maxIdx = i;
        }
    }

    // Output in order of occurrence
    if (minIdx <= maxIdx) {
        outputX[outIdx] = inputX[minIdx];
        outputY[outIdx] = minY;
        outputIndices[outIdx] = minIdx;
        outputX[outIdx + 1] = inputX[maxIdx];
        outputY[outIdx + 1] = maxY;
        outputIndices[outIdx + 1] = maxIdx;
    } else {
        outputX[outIdx] = inputX[maxIdx];
        outputY[outIdx] = maxY;
        outputIndices[outIdx] = maxIdx;
        outputX[outIdx + 1] = inputX[minIdx];
        outputY[outIdx + 1] = minY;
        outputIndices[outIdx + 1] = minIdx;
    }
}

// LTTB (Largest Triangle Three Buckets) - per-bucket kernel
// Each thread computes the selected point for one bucket
kernel void decimate_lttb(
    constant float* inputX [[buffer(0)]],
    constant float* inputY [[buffer(1)]],
    device float* outputX [[buffer(2)]],
    device float* outputY [[buffer(3)]],
    device uint* outputIndices [[buffer(4)]],
    constant DecimationUniforms& uniforms [[buffer(5)]],
    device uint* prevSelected [[buffer(6)]],   // selected index from previous bucket
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= uniforms.outputCount) return;

    // First and last points are always kept
    if (gid == 0) {
        outputX[0] = inputX[0];
        outputY[0] = inputY[0];
        outputIndices[0] = 0;
        prevSelected[0] = 0;
        return;
    }
    if (gid == uniforms.outputCount - 1) {
        uint last = uniforms.inputCount - 1;
        outputX[gid] = inputX[last];
        outputY[gid] = inputY[last];
        outputIndices[gid] = last;
        return;
    }

    uint bucketSize = (uniforms.inputCount - 2) / (uniforms.outputCount - 2);
    uint start = 1 + (gid - 1) * bucketSize;
    uint end = min(start + bucketSize, uniforms.inputCount - 1);

    // Next bucket average (the "third point" in the triangle)
    uint nextStart = end;
    uint nextEnd = min(nextStart + bucketSize, uniforms.inputCount - 1);
    float avgX = 0.0;
    float avgY = 0.0;
    uint nextCount = nextEnd - nextStart;
    for (uint i = nextStart; i < nextEnd; i++) {
        avgX += inputX[i];
        avgY += inputY[i];
    }
    if (nextCount > 0) {
        avgX /= float(nextCount);
        avgY /= float(nextCount);
    }

    // Previous selected point
    uint prevIdx = prevSelected[gid - 1];
    float prevX = inputX[prevIdx];
    float prevY = inputY[prevIdx];

    // Find point in current bucket that maximizes triangle area
    float maxArea = -1.0;
    uint bestIdx = start;

    for (uint i = start; i < end; i++) {
        float area = abs(
            (prevX - avgX) * (inputY[i] - prevY) -
            (prevX - inputX[i]) * (avgY - prevY)
        );
        if (area > maxArea) {
            maxArea = area;
            bestIdx = i;
        }
    }

    outputX[gid] = inputX[bestIdx];
    outputY[gid] = inputY[bestIdx];
    outputIndices[gid] = bestIdx;
    prevSelected[gid] = bestIdx;
}
)METAL";

} // namespace shaders
} // namespace nv

#endif
