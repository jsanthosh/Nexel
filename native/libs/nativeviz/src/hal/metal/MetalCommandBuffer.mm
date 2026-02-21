#import "MetalCommandBuffer.h"
#import "MetalDevice.h"
#import "MetalBuffer.h"
#import "MetalTexture.h"
#import "MetalPipeline.h"
#import <Metal/Metal.h>

namespace nv {

static MTLPrimitiveType toMTL(NVPrimitiveType type) {
    switch (type) {
        case NV_PRIMITIVE_TRIANGLE: return MTLPrimitiveTypeTriangle;
        case NV_PRIMITIVE_TRIANGLE_STRIP: return MTLPrimitiveTypeTriangleStrip;
        case NV_PRIMITIVE_LINE: return MTLPrimitiveTypeLine;
        case NV_PRIMITIVE_LINE_STRIP: return MTLPrimitiveTypeLineStrip;
        case NV_PRIMITIVE_POINT: return MTLPrimitiveTypePoint;
    }
    return MTLPrimitiveTypeTriangle;
}

MetalCommandBuffer::MetalCommandBuffer(MetalDevice* device) : m_device(device) {
    @autoreleasepool {
        auto* queue = (__bridge id<MTLCommandQueue>)device->metalCommandQueue();
        m_commandBuffer = (__bridge_retained void*)[queue commandBuffer];
    }
}

MetalCommandBuffer::~MetalCommandBuffer() {
    @autoreleasepool {
        if (m_renderEncoder) CFRelease(m_renderEncoder);
        if (m_computeEncoder) CFRelease(m_computeEncoder);
        if (m_commandBuffer) CFRelease(m_commandBuffer);
    }
}

void MetalCommandBuffer::beginRenderPass(NVTexture* target, NVColor clearColor) {
    @autoreleasepool {
        auto* texture = static_cast<MetalTexture*>(target);
        auto* mtlTexture = (__bridge id<MTLTexture>)texture->nativeHandle();

        MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
        rpd.colorAttachments[0].texture = mtlTexture;
        rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
        rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
        rpd.colorAttachments[0].clearColor = MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

        auto* cmdBuf = (__bridge id<MTLCommandBuffer>)m_commandBuffer;
        id<MTLRenderCommandEncoder> encoder = [cmdBuf renderCommandEncoderWithDescriptor:rpd];
        m_renderEncoder = (__bridge_retained void*)encoder;
    }
}

void MetalCommandBuffer::endRenderPass() {
    @autoreleasepool {
        if (m_renderEncoder) {
            auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
            [encoder endEncoding];
            CFRelease(m_renderEncoder);
            m_renderEncoder = nullptr;
        }
        if (m_computeEncoder) {
            auto* encoder = (__bridge id<MTLComputeCommandEncoder>)m_computeEncoder;
            [encoder endEncoding];
            CFRelease(m_computeEncoder);
            m_computeEncoder = nullptr;
        }
    }
}

void MetalCommandBuffer::setRenderPipeline(NVPipeline* pipeline) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        auto* pso = (__bridge id<MTLRenderPipelineState>)pipeline->nativeHandle();
        [encoder setRenderPipelineState:pso];
    }
}

void MetalCommandBuffer::setComputePipeline(NVPipeline* pipeline) {
    @autoreleasepool {
        // End render encoder if active
        if (m_renderEncoder) {
            endRenderPass();
        }
        auto* cmdBuf = (__bridge id<MTLCommandBuffer>)m_commandBuffer;
        id<MTLComputeCommandEncoder> encoder = [cmdBuf computeCommandEncoder];
        m_computeEncoder = (__bridge_retained void*)encoder;

        auto* pso = (__bridge id<MTLComputePipelineState>)pipeline->nativeHandle();
        [encoder setComputePipelineState:pso];
    }
}

void MetalCommandBuffer::setViewport(NVViewport viewport) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        MTLViewport vp = {
            .originX = (double)viewport.x,
            .originY = (double)viewport.y,
            .width = (double)viewport.width,
            .height = (double)viewport.height,
            .znear = 0.0,
            .zfar = 1.0
        };
        [encoder setViewport:vp];
    }
}

void MetalCommandBuffer::setScissor(NVRect rect) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        MTLScissorRect sr = {
            .x = (NSUInteger)rect.x,
            .y = (NSUInteger)rect.y,
            .width = (NSUInteger)rect.width,
            .height = (NSUInteger)rect.height
        };
        [encoder setScissorRect:sr];
    }
}

void MetalCommandBuffer::setVertexBuffer(NVBuffer* buffer, int index, size_t offset) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        auto* mtlBuffer = (__bridge id<MTLBuffer>)static_cast<MetalBuffer*>(buffer)->nativeHandle();
        [encoder setVertexBuffer:mtlBuffer offset:offset atIndex:index];
    }
}

void MetalCommandBuffer::setFragmentBuffer(NVBuffer* buffer, int index, size_t offset) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        auto* mtlBuffer = (__bridge id<MTLBuffer>)static_cast<MetalBuffer*>(buffer)->nativeHandle();
        [encoder setFragmentBuffer:mtlBuffer offset:offset atIndex:index];
    }
}

void MetalCommandBuffer::setComputeBuffer(NVBuffer* buffer, int index, size_t offset) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLComputeCommandEncoder>)m_computeEncoder;
        auto* mtlBuffer = (__bridge id<MTLBuffer>)static_cast<MetalBuffer*>(buffer)->nativeHandle();
        [encoder setBuffer:mtlBuffer offset:offset atIndex:index];
    }
}

void MetalCommandBuffer::setFragmentTexture(NVTexture* texture, int index) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        auto* mtlTexture = (__bridge id<MTLTexture>)texture->nativeHandle();
        [encoder setFragmentTexture:mtlTexture atIndex:index];
    }
}

void MetalCommandBuffer::setComputeTexture(NVTexture* texture, int index) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLComputeCommandEncoder>)m_computeEncoder;
        auto* mtlTexture = (__bridge id<MTLTexture>)texture->nativeHandle();
        [encoder setTexture:mtlTexture atIndex:index];
    }
}

void MetalCommandBuffer::draw(NVPrimitiveType type, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        [encoder drawPrimitives:toMTL(type)
                    vertexStart:firstVertex
                    vertexCount:vertexCount
                  instanceCount:instanceCount];
    }
}

void MetalCommandBuffer::drawIndexed(NVPrimitiveType type, NVBuffer* indexBuffer, uint32_t indexCount, uint32_t instanceCount) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
        auto* mtlBuffer = (__bridge id<MTLBuffer>)static_cast<MetalBuffer*>(indexBuffer)->nativeHandle();
        [encoder drawIndexedPrimitives:toMTL(type)
                            indexCount:indexCount
                             indexType:MTLIndexTypeUInt32
                           indexBuffer:mtlBuffer
                     indexBufferOffset:0
                         instanceCount:instanceCount];
    }
}

void MetalCommandBuffer::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) {
    @autoreleasepool {
        auto* encoder = (__bridge id<MTLComputeCommandEncoder>)m_computeEncoder;
        MTLSize threadgroups = MTLSizeMake(groupsX, groupsY, groupsZ);
        MTLSize threadsPerGroup = MTLSizeMake(256, 1, 1); // Default; pipelines can override
        [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerGroup];
    }
}

void MetalCommandBuffer::blitTexture(NVTexture* src, NVRect srcRect, NVTexture* dst, NVRect dstRect) {
    @autoreleasepool {
        auto* cmdBuf = (__bridge id<MTLCommandBuffer>)m_commandBuffer;
        id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];

        auto* srcTex = (__bridge id<MTLTexture>)src->nativeHandle();
        auto* dstTex = (__bridge id<MTLTexture>)dst->nativeHandle();

        [blit copyFromTexture:srcTex
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake((NSUInteger)srcRect.x, (NSUInteger)srcRect.y, 0)
                   sourceSize:MTLSizeMake((NSUInteger)srcRect.width, (NSUInteger)srcRect.height, 1)
                    toTexture:dstTex
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake((NSUInteger)dstRect.x, (NSUInteger)dstRect.y, 0)];

        [blit endEncoding];
    }
}

void MetalCommandBuffer::commit() {
    @autoreleasepool {
        // End any active encoders
        if (m_renderEncoder) {
            auto* encoder = (__bridge id<MTLRenderCommandEncoder>)m_renderEncoder;
            [encoder endEncoding];
            CFRelease(m_renderEncoder);
            m_renderEncoder = nullptr;
        }
        if (m_computeEncoder) {
            auto* encoder = (__bridge id<MTLComputeCommandEncoder>)m_computeEncoder;
            [encoder endEncoding];
            CFRelease(m_computeEncoder);
            m_computeEncoder = nullptr;
        }

        auto* cmdBuf = (__bridge id<MTLCommandBuffer>)m_commandBuffer;
        [cmdBuf commit];
    }
}

void MetalCommandBuffer::waitUntilCompleted() {
    @autoreleasepool {
        auto* cmdBuf = (__bridge id<MTLCommandBuffer>)m_commandBuffer;
        [cmdBuf waitUntilCompleted];
    }
}

} // namespace nv
