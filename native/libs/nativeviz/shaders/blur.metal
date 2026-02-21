#include <metal_stdlib>
using namespace metal;

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
