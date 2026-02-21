#ifndef NV_METAL_COMMAND_BUFFER_H
#define NV_METAL_COMMAND_BUFFER_H

#include "../NVCommandBuffer.h"

namespace nv {

class MetalDevice;

class MetalCommandBuffer : public NVCommandBuffer {
public:
    explicit MetalCommandBuffer(MetalDevice* device);
    ~MetalCommandBuffer() override;

    void beginRenderPass(NVTexture* target, NVColor clearColor) override;
    void endRenderPass() override;

    void setRenderPipeline(NVPipeline* pipeline) override;
    void setComputePipeline(NVPipeline* pipeline) override;
    void setViewport(NVViewport viewport) override;
    void setScissor(NVRect rect) override;

    void setVertexBuffer(NVBuffer* buffer, int index, size_t offset) override;
    void setFragmentBuffer(NVBuffer* buffer, int index, size_t offset) override;
    void setComputeBuffer(NVBuffer* buffer, int index, size_t offset) override;

    void setFragmentTexture(NVTexture* texture, int index) override;
    void setComputeTexture(NVTexture* texture, int index) override;

    void draw(NVPrimitiveType type, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) override;
    void drawIndexed(NVPrimitiveType type, NVBuffer* indexBuffer, uint32_t indexCount, uint32_t instanceCount) override;

    void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;

    void blitTexture(NVTexture* src, NVRect srcRect, NVTexture* dst, NVRect dstRect) override;

    void commit() override;
    void waitUntilCompleted() override;

    // Metal-specific
    void* metalCommandBuffer() const { return m_commandBuffer; }

private:
    MetalDevice* m_device;
    void* m_commandBuffer = nullptr;   // id<MTLCommandBuffer>
    void* m_renderEncoder = nullptr;   // id<MTLRenderCommandEncoder>
    void* m_computeEncoder = nullptr;  // id<MTLComputeCommandEncoder>
};

} // namespace nv

#endif // NV_METAL_COMMAND_BUFFER_H
