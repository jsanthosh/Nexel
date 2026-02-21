#ifndef NV_METAL_TEXTURE_H
#define NV_METAL_TEXTURE_H

#include "../NVTexture.h"

namespace nv {

class MetalDevice;

class MetalTexture : public NVTexture {
public:
    MetalTexture(MetalDevice* device, int width, int height, NVPixelFormat format);
    ~MetalTexture() override;

    int width() const override { return m_width; }
    int height() const override { return m_height; }
    NVPixelFormat format() const override { return m_format; }

    void upload(const void* data, size_t bytesPerRow) override;

    bool isRenderTarget() const override { return m_isRenderTarget; }
    void setAsRenderTarget(bool value) override { m_isRenderTarget = value; }

    void* nativeHandle() const override { return m_texture; }

private:
    void* m_texture = nullptr; // id<MTLTexture>
    int m_width;
    int m_height;
    NVPixelFormat m_format;
    bool m_isRenderTarget = false;
};

} // namespace nv

#endif // NV_METAL_TEXTURE_H
