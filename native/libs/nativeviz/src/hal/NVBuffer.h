#ifndef NV_BUFFER_H
#define NV_BUFFER_H

#include "nativeviz/nv_types.h"
#include <cstddef>

namespace nv {

class NVBuffer {
public:
    virtual ~NVBuffer() = default;

    // Data access
    virtual void* contents() = 0;
    virtual const void* contents() const = 0;
    virtual size_t size() const = 0;

    // Upload data (copies from CPU memory to GPU buffer)
    virtual void upload(const void* data, size_t size, size_t offset = 0) = 0;

    // For shared/managed buffers: notify GPU of CPU writes
    virtual void didModifyRange(size_t offset, size_t length) = 0;

    NVBufferUsage usage() const { return m_usage; }

protected:
    NVBuffer(NVBufferUsage usage) : m_usage(usage) {}
    NVBufferUsage m_usage;
};

} // namespace nv

#endif // NV_BUFFER_H
