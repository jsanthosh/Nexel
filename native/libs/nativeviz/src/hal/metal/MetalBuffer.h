#ifndef NV_METAL_BUFFER_H
#define NV_METAL_BUFFER_H

#include "../NVBuffer.h"

namespace nv {

class MetalDevice;

class MetalBuffer : public NVBuffer {
public:
    MetalBuffer(MetalDevice* device, size_t size, NVBufferUsage usage);
    ~MetalBuffer() override;

    void* contents() override;
    const void* contents() const override;
    size_t size() const override { return m_size; }

    void upload(const void* data, size_t size, size_t offset) override;
    void didModifyRange(size_t offset, size_t length) override;

    void* nativeHandle() const { return m_buffer; }

private:
    void* m_buffer = nullptr; // id<MTLBuffer>
    size_t m_size;
};

} // namespace nv

#endif // NV_METAL_BUFFER_H
