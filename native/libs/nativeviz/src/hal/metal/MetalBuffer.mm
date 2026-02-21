#import "MetalBuffer.h"
#import "MetalDevice.h"
#import <Metal/Metal.h>
#include <cstring>

namespace nv {

MetalBuffer::MetalBuffer(MetalDevice* device, size_t size, NVBufferUsage usage)
    : NVBuffer(usage), m_size(size) {
    @autoreleasepool {
        auto* mtlDevice = (__bridge id<MTLDevice>)device->metalDevice();

        // Use shared storage for vertex/uniform buffers (CPU+GPU access)
        // Use private storage for large storage buffers (GPU-only)
        MTLResourceOptions options;
        if (usage == NV_BUFFER_USAGE_STORAGE && size > 1024 * 1024) {
            options = MTLResourceStorageModePrivate;
        } else {
            options = MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
        }

        id<MTLBuffer> buffer = [mtlDevice newBufferWithLength:size options:options];
        m_buffer = (__bridge_retained void*)buffer;
    }
}

MetalBuffer::~MetalBuffer() {
    @autoreleasepool {
        if (m_buffer) CFRelease(m_buffer);
    }
}

void* MetalBuffer::contents() {
    @autoreleasepool {
        auto* buffer = (__bridge id<MTLBuffer>)m_buffer;
        return [buffer contents];
    }
}

const void* MetalBuffer::contents() const {
    @autoreleasepool {
        auto* buffer = (__bridge id<MTLBuffer>)m_buffer;
        return [buffer contents];
    }
}

void MetalBuffer::upload(const void* data, size_t size, size_t offset) {
    @autoreleasepool {
        auto* buffer = (__bridge id<MTLBuffer>)m_buffer;
        void* dst = static_cast<uint8_t*>([buffer contents]) + offset;
        std::memcpy(dst, data, size);
    }
}

void MetalBuffer::didModifyRange(size_t offset, size_t length) {
    @autoreleasepool {
        auto* buffer = (__bridge id<MTLBuffer>)m_buffer;
        [buffer didModifyRange:NSMakeRange(offset, length)];
    }
}

} // namespace nv
