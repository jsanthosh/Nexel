#import "MetalTexture.h"
#import "MetalDevice.h"
#import <Metal/Metal.h>

namespace nv {

static MTLPixelFormat toMTLPixelFormat(NVPixelFormat fmt) {
    switch (fmt) {
        case NV_PIXEL_FORMAT_RGBA8_UNORM: return MTLPixelFormatRGBA8Unorm;
        case NV_PIXEL_FORMAT_BGRA8_UNORM: return MTLPixelFormatBGRA8Unorm;
        case NV_PIXEL_FORMAT_R8_UNORM: return MTLPixelFormatR8Unorm;
        case NV_PIXEL_FORMAT_RG16_FLOAT: return MTLPixelFormatRG16Float;
        case NV_PIXEL_FORMAT_RGBA16_FLOAT: return MTLPixelFormatRGBA16Float;
        case NV_PIXEL_FORMAT_RGBA32_FLOAT: return MTLPixelFormatRGBA32Float;
        case NV_PIXEL_FORMAT_DEPTH32_FLOAT: return MTLPixelFormatDepth32Float;
    }
    return MTLPixelFormatBGRA8Unorm;
}

MetalTexture::MetalTexture(MetalDevice* device, int width, int height, NVPixelFormat format)
    : m_width(width), m_height(height), m_format(format) {
    @autoreleasepool {
        auto* mtlDevice = (__bridge id<MTLDevice>)device->metalDevice();

        MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:toMTLPixelFormat(format)
                                                                                       width:width
                                                                                      height:height
                                                                                   mipmapped:NO];
        desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        desc.storageMode = MTLStorageModePrivate;

        // For render targets, we need private storage; for uploads, use managed
        if (format != NV_PIXEL_FORMAT_DEPTH32_FLOAT) {
            desc.storageMode = MTLStorageModeManaged;
        }

        id<MTLTexture> texture = [mtlDevice newTextureWithDescriptor:desc];
        m_texture = (__bridge_retained void*)texture;
    }
}

MetalTexture::~MetalTexture() {
    @autoreleasepool {
        if (m_texture) CFRelease(m_texture);
    }
}

void MetalTexture::upload(const void* data, size_t bytesPerRow) {
    @autoreleasepool {
        auto* texture = (__bridge id<MTLTexture>)m_texture;
        MTLRegion region = MTLRegionMake2D(0, 0, m_width, m_height);
        [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
    }
}

} // namespace nv
