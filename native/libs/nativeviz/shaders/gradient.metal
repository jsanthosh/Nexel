#include <metal_stdlib>
using namespace metal;

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
