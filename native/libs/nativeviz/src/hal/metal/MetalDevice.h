#ifndef NV_METAL_DEVICE_H
#define NV_METAL_DEVICE_H

#include "../NVDevice.h"

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

namespace nv {

class MetalDevice : public NVDevice {
public:
    MetalDevice();
    ~MetalDevice() override;

    std::unique_ptr<NVCommandBuffer> createCommandBuffer() override;
    std::unique_ptr<NVBuffer> createBuffer(size_t size, NVBufferUsage usage) override;
    std::unique_ptr<NVTexture> createTexture(int width, int height, NVPixelFormat format) override;
    std::unique_ptr<NVPipeline> createRenderPipeline(const NVPipelineDesc& desc) override;
    std::unique_ptr<NVPipeline> createComputePipeline(const NVComputePipelineDesc& desc) override;

    void submit(NVCommandBuffer* cmd) override;
    void waitIdle() override;

    std::string deviceName() const override;
    NVBackend backend() const override { return NV_BACKEND_METAL; }
    size_t maxBufferSize() const override;

    // Metal-specific accessors
    void* metalDevice() const { return m_device; }
    void* metalCommandQueue() const { return m_commandQueue; }
    void* metalLibrary() const { return m_defaultLibrary; }

private:
    void* m_device = nullptr;          // id<MTLDevice>
    void* m_commandQueue = nullptr;    // id<MTLCommandQueue>
    void* m_defaultLibrary = nullptr;  // id<MTLLibrary>
};

} // namespace nv

#endif // NV_METAL_DEVICE_H
