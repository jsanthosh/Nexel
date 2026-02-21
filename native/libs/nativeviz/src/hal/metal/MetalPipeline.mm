#import "MetalPipeline.h"
#import "MetalDevice.h"
#import <Metal/Metal.h>

namespace nv {

static MTLPixelFormat toMTLPixelFormat(NVPixelFormat fmt) {
    switch (fmt) {
        case NV_PIXEL_FORMAT_RGBA8_UNORM: return MTLPixelFormatRGBA8Unorm;
        case NV_PIXEL_FORMAT_BGRA8_UNORM: return MTLPixelFormatBGRA8Unorm;
        case NV_PIXEL_FORMAT_RGBA16_FLOAT: return MTLPixelFormatRGBA16Float;
        default: return MTLPixelFormatBGRA8Unorm;
    }
}

MetalPipeline::~MetalPipeline() {
    @autoreleasepool {
        if (m_pipeline) CFRelease(m_pipeline);
    }
}

std::unique_ptr<MetalPipeline> MetalPipeline::createRender(MetalDevice* device, const NVPipelineDesc& desc) {
    @autoreleasepool {
        auto* mtlDevice = (__bridge id<MTLDevice>)device->metalDevice();
        auto* library = (__bridge id<MTLLibrary>)device->metalLibrary();

        if (!library) {
            // Compile shaders from source at runtime as fallback
            return nullptr;
        }

        NSString* vertName = [NSString stringWithUTF8String:desc.vertexFunction.c_str()];
        NSString* fragName = [NSString stringWithUTF8String:desc.fragmentFunction.c_str()];

        id<MTLFunction> vertexFunc = [library newFunctionWithName:vertName];
        id<MTLFunction> fragmentFunc = [library newFunctionWithName:fragName];

        if (!vertexFunc || !fragmentFunc) {
            return nullptr;
        }

        MTLRenderPipelineDescriptor* pipeDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipeDesc.vertexFunction = vertexFunc;
        pipeDesc.fragmentFunction = fragmentFunc;
        pipeDesc.colorAttachments[0].pixelFormat = toMTLPixelFormat(desc.colorFormat);

        // Configure blending
        if (desc.blendMode != NV_BLEND_NONE) {
            pipeDesc.colorAttachments[0].blendingEnabled = YES;

            switch (desc.blendMode) {
                case NV_BLEND_ALPHA:
                    pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                    pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    pipeDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
                    pipeDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    break;
                case NV_BLEND_PREMULTIPLIED_ALPHA:
                    pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
                    pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    pipeDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
                    pipeDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                    break;
                case NV_BLEND_ADDITIVE:
                    pipeDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                    pipeDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
                    pipeDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
                    pipeDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
                    break;
                default:
                    break;
            }
        }

        NSError* error = nil;
        id<MTLRenderPipelineState> pso = [mtlDevice newRenderPipelineStateWithDescriptor:pipeDesc error:&error];
        if (!pso) {
            NSLog(@"NativeViz: Failed to create render pipeline: %@", error);
            return nullptr;
        }

        auto pipeline = std::unique_ptr<MetalPipeline>(new MetalPipeline());
        pipeline->m_pipeline = (__bridge_retained void*)pso;
        pipeline->m_isCompute = false;
        return pipeline;
    }
}

std::unique_ptr<MetalPipeline> MetalPipeline::createCompute(MetalDevice* device, const NVComputePipelineDesc& desc) {
    @autoreleasepool {
        auto* mtlDevice = (__bridge id<MTLDevice>)device->metalDevice();
        auto* library = (__bridge id<MTLLibrary>)device->metalLibrary();

        if (!library) return nullptr;

        NSString* funcName = [NSString stringWithUTF8String:desc.computeFunction.c_str()];
        id<MTLFunction> func = [library newFunctionWithName:funcName];
        if (!func) return nullptr;

        NSError* error = nil;
        id<MTLComputePipelineState> pso = [mtlDevice newComputePipelineStateWithFunction:func error:&error];
        if (!pso) {
            NSLog(@"NativeViz: Failed to create compute pipeline: %@", error);
            return nullptr;
        }

        auto pipeline = std::unique_ptr<MetalPipeline>(new MetalPipeline());
        pipeline->m_pipeline = (__bridge_retained void*)pso;
        pipeline->m_isCompute = true;
        return pipeline;
    }
}

} // namespace nv
