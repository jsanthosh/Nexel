#include "NVGradient.h"
#include "../hal/NVDevice.h"
#include "../hal/NVTexture.h"
#include <cmath>
#include <cstring>
#include <functional>

namespace nv {

uint64_t NVGradientDesc::hash() const {
    uint64_t h = static_cast<uint64_t>(type) * 2654435761ULL;
    h ^= std::hash<float>{}(start.x) * 2246822519ULL;
    h ^= std::hash<float>{}(start.y) * 3266489917ULL;
    h ^= std::hash<float>{}(end.x) * 668265263ULL;
    h ^= std::hash<float>{}(end.y) * 374761393ULL;
    h ^= std::hash<float>{}(radius) * 2654435761ULL;
    for (const auto& stop : stops) {
        h ^= std::hash<float>{}(stop.position) * 2246822519ULL;
        h ^= std::hash<float>{}(stop.color.r) * 3266489917ULL;
        h ^= std::hash<float>{}(stop.color.g) * 668265263ULL;
        h ^= std::hash<float>{}(stop.color.b) * 374761393ULL;
        h ^= std::hash<float>{}(stop.color.a) * 2654435761ULL;
    }
    return h;
}

NVGradient::NVGradient(NVDevice* device) : m_device(device) {}
NVGradient::~NVGradient() = default;

NVTexture* NVGradient::getTexture(const NVGradientDesc& desc, int width, int height) {
    // For radial/conic gradients, use 2D texture
    if (desc.type == NV_GRADIENT_RADIAL || desc.type == NV_GRADIENT_CONIC) {
        height = width;
    }

    uint64_t key = desc.hash() ^ (static_cast<uint64_t>(width) << 32) ^ height;
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        return it->second.get();
    }

    // Generate on CPU and upload
    std::vector<uint8_t> pixels(width * height * 4);
    generateCPU(desc, pixels, width, height);

    auto texture = m_device->createTexture(width, height, NV_PIXEL_FORMAT_RGBA8_UNORM);
    texture->upload(pixels.data(), width * 4);

    auto* ptr = texture.get();
    m_cache[key] = std::move(texture);
    return ptr;
}

void NVGradient::clearCache() {
    m_cache.clear();
}

static NVColor interpolateStops(const std::vector<NVGradientStop>& stops, float t) {
    if (stops.empty()) return {0, 0, 0, 1};
    if (t <= stops.front().position) return stops.front().color;
    if (t >= stops.back().position) return stops.back().color;

    for (size_t i = 1; i < stops.size(); i++) {
        if (t <= stops[i].position) {
            float range = stops[i].position - stops[i-1].position;
            float localT = (range > 0) ? (t - stops[i-1].position) / range : 0;
            const auto& a = stops[i-1].color;
            const auto& b = stops[i].color;
            return {
                a.r + (b.r - a.r) * localT,
                a.g + (b.g - a.g) * localT,
                a.b + (b.b - a.b) * localT,
                a.a + (b.a - a.a) * localT
            };
        }
    }
    return stops.back().color;
}

void NVGradient::generateCPU(const NVGradientDesc& desc, std::vector<uint8_t>& pixels, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float u = (width > 1) ? static_cast<float>(x) / (width - 1) : 0;
            float v = (height > 1) ? static_cast<float>(y) / (height - 1) : 0;
            float t = 0;

            switch (desc.type) {
                case NV_GRADIENT_LINEAR: {
                    float dx = desc.end.x - desc.start.x;
                    float dy = desc.end.y - desc.start.y;
                    float len = std::sqrt(dx * dx + dy * dy);
                    if (len > 0) {
                        float nx = dx / len, ny = dy / len;
                        t = ((u - desc.start.x) * nx + (v - desc.start.y) * ny) / len;
                    }
                    break;
                }
                case NV_GRADIENT_RADIAL: {
                    float dx = u - desc.start.x;
                    float dy = v - desc.start.y;
                    t = std::sqrt(dx * dx + dy * dy) / std::max(desc.radius, 0.001f);
                    break;
                }
                case NV_GRADIENT_CONIC: {
                    float dx = u - desc.start.x;
                    float dy = v - desc.start.y;
                    float a = std::atan2(dy, dx) - desc.angle;
                    t = std::fmod(a / (2.0f * M_PI) + 1.0f, 1.0f);
                    break;
                }
            }

            t = std::clamp(t, 0.0f, 1.0f);
            NVColor c = interpolateStops(desc.stops, t);

            int idx = (y * width + x) * 4;
            pixels[idx + 0] = static_cast<uint8_t>(c.r * 255);
            pixels[idx + 1] = static_cast<uint8_t>(c.g * 255);
            pixels[idx + 2] = static_cast<uint8_t>(c.b * 255);
            pixels[idx + 3] = static_cast<uint8_t>(c.a * 255);
        }
    }
}

} // namespace nv
