#ifndef NV_GRADIENT_H
#define NV_GRADIENT_H

#include "nativeviz/nv_types.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace nv {

class NVDevice;
class NVTexture;

struct NVGradientDesc {
    NVGradientType type = NV_GRADIENT_LINEAR;
    std::vector<NVGradientStop> stops;
    NVPoint start = {0, 0};       // linear: start; radial/conic: center
    NVPoint end = {1, 0};         // linear: end
    float radius = 1.0f;          // radial: radius
    float angle = 0;              // conic: start angle

    // Generate a hash for caching
    uint64_t hash() const;
};

class NVGradient {
public:
    explicit NVGradient(NVDevice* device);
    ~NVGradient();

    // Get or create a gradient texture (cached)
    NVTexture* getTexture(const NVGradientDesc& desc, int width = 256, int height = 1);

    // Clear the cache
    void clearCache();

private:
    NVDevice* m_device;
    std::unordered_map<uint64_t, std::unique_ptr<NVTexture>> m_cache;

    // CPU fallback for generating gradient pixels
    void generateCPU(const NVGradientDesc& desc, std::vector<uint8_t>& pixels, int width, int height);
};

} // namespace nv

#endif // NV_GRADIENT_H
