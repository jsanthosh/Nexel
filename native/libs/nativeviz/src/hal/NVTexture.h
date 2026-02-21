#ifndef NV_TEXTURE_H
#define NV_TEXTURE_H

#include "nativeviz/nv_types.h"
#include <cstddef>

namespace nv {

class NVTexture {
public:
    virtual ~NVTexture() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual NVPixelFormat format() const = 0;

    // Upload pixel data from CPU
    virtual void upload(const void* data, size_t bytesPerRow) = 0;

    // Mark as render target
    virtual bool isRenderTarget() const = 0;
    virtual void setAsRenderTarget(bool value) = 0;

    // Native handle for platform interop
    virtual void* nativeHandle() const = 0;

protected:
    NVTexture() = default;
};

} // namespace nv

#endif // NV_TEXTURE_H
