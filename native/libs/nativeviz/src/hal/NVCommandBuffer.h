#ifndef NV_COMMAND_BUFFER_H
#define NV_COMMAND_BUFFER_H

#include "nativeviz/nv_types.h"

namespace nv {

class NVBuffer;
class NVTexture;
class NVPipeline;

class NVCommandBuffer {
public:
    virtual ~NVCommandBuffer() = default;

    // Render pass
    virtual void beginRenderPass(NVTexture* target, NVColor clearColor = {0, 0, 0, 1}) = 0;
    virtual void endRenderPass() = 0;

    // Pipeline state
    virtual void setRenderPipeline(NVPipeline* pipeline) = 0;
    virtual void setComputePipeline(NVPipeline* pipeline) = 0;
    virtual void setViewport(NVViewport viewport) = 0;
    virtual void setScissor(NVRect rect) = 0;

    // Vertex/Index buffers
    virtual void setVertexBuffer(NVBuffer* buffer, int index, size_t offset = 0) = 0;
    virtual void setFragmentBuffer(NVBuffer* buffer, int index, size_t offset = 0) = 0;
    virtual void setComputeBuffer(NVBuffer* buffer, int index, size_t offset = 0) = 0;

    // Textures
    virtual void setFragmentTexture(NVTexture* texture, int index) = 0;
    virtual void setComputeTexture(NVTexture* texture, int index) = 0;

    // Draw commands
    virtual void draw(NVPrimitiveType type, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0) = 0;
    virtual void drawIndexed(NVPrimitiveType type, NVBuffer* indexBuffer, uint32_t indexCount, uint32_t instanceCount = 1) = 0;

    // Compute dispatch
    virtual void dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;

    // Blit
    virtual void blitTexture(NVTexture* src, NVRect srcRect, NVTexture* dst, NVRect dstRect) = 0;

    // Commit
    virtual void commit() = 0;
    virtual void waitUntilCompleted() = 0;

protected:
    NVCommandBuffer() = default;
};

} // namespace nv

#endif // NV_COMMAND_BUFFER_H
