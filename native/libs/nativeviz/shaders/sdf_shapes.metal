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
