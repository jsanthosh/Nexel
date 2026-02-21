#ifndef NV_DEVICE_H
#define NV_DEVICE_H

#include "nativeviz/nv_types.h"
#include <memory>
#include <string>

namespace nv {

class NVCommandBuffer;
class NVBuffer;
class NVTexture;
class NVPipeline;

struct NVPipelineDesc {
    std::string vertexFunction;
    std::string fragmentFunction;
    NVPixelFormat colorFormat = NV_PIXEL_FORMAT_BGRA8_UNORM;
    NVBlendMode blendMode = NV_BLEND_ALPHA;
    NVPrimitiveType primitiveType = NV_PRIMITIVE_TRIANGLE;
    bool depthTestEnabled = false;
};

struct NVComputePipelineDesc {
    std::string computeFunction;
};

class NVDevice {
public:
    virtual ~NVDevice() = default;

    // Factory
    static std::unique_ptr<NVDevice> create(NVBackend backend = NV_BACKEND_AUTO);

    // Resource creation
    virtual std::unique_ptr<NVCommandBuffer> createCommandBuffer() = 0;
    virtual std::unique_ptr<NVBuffer> createBuffer(size_t size, NVBufferUsage usage) = 0;
    virtual std::unique_ptr<NVTexture> createTexture(int width, int height, NVPixelFormat format) = 0;
    virtual std::unique_ptr<NVPipeline> createRenderPipeline(const NVPipelineDesc& desc) = 0;
    virtual std::unique_ptr<NVPipeline> createComputePipeline(const NVComputePipelineDesc& desc) = 0;

    // Command submission
    virtual void submit(NVCommandBuffer* cmd) = 0;
    virtual void waitIdle() = 0;

    // Info
    virtual std::string deviceName() const = 0;
    virtual NVBackend backend() const = 0;
    virtual size_t maxBufferSize() const = 0;

protected:
    NVDevice() = default;
};

} // namespace nv

#endif // NV_DEVICE_H
