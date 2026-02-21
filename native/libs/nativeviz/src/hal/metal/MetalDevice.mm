#import "MetalDevice.h"
#import "MetalCommandBuffer.h"
#import "MetalBuffer.h"
#import "MetalTexture.h"
#import "MetalPipeline.h"
#import "MetalShaderSource.h"
#import <Metal/Metal.h>

namespace nv {

// --- NVDevice factory ---

std::unique_ptr<NVDevice> NVDevice::create(NVBackend backend) {
    if (backend == NV_BACKEND_AUTO || backend == NV_BACKEND_METAL) {
        return std::make_unique<MetalDevice>();
    }
    return nullptr; // Other backends not yet implemented
}

// --- MetalDevice ---

MetalDevice::MetalDevice() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            throw std::runtime_error("Metal is not supported on this device");
        }
        m_device = (__bridge_retained void*)device;
        m_commandQueue = (__bridge_retained void*)[device newCommandQueue];

        NSError* error = nil;

#ifdef NV_USE_PRECOMPILED_SHADERS
        // Try pre-compiled metallib first
        NSString* libPath = [[NSBundle mainBundle] pathForResource:@"nativeviz_shaders" ofType:@"metallib"];
        if (libPath) {
            id<MTLLibrary> lib = [device newLibraryWithFile:libPath error:&error];
            if (lib) {
                m_defaultLibrary = (__bridge_retained void*)lib;
            }
        }
#endif

        // Runtime compilation from embedded source
        if (!m_defaultLibrary) {
            NSString* source = [NSString stringWithUTF8String:shaders::kAllShaders.data()];
            MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
            options.languageVersion = MTLLanguageVersion2_4;
            options.mathMode = MTLMathModeFast;

            id<MTLLibrary> lib = [device newLibraryWithSource:source options:options error:&error];
            if (lib) {
                m_defaultLibrary = (__bridge_retained void*)lib;
            } else {
                NSLog(@"NativeViz: Failed to compile shaders: %@", error);
            }
        }
    }
}

MetalDevice::~MetalDevice() {
    @autoreleasepool {
        if (m_defaultLibrary) CFRelease(m_defaultLibrary);
        if (m_commandQueue) CFRelease(m_commandQueue);
        if (m_device) CFRelease(m_device);
    }
}

std::unique_ptr<NVCommandBuffer> MetalDevice::createCommandBuffer() {
    return std::make_unique<MetalCommandBuffer>(this);
}

std::unique_ptr<NVBuffer> MetalDevice::createBuffer(size_t size, NVBufferUsage usage) {
    return std::make_unique<MetalBuffer>(this, size, usage);
}

std::unique_ptr<NVTexture> MetalDevice::createTexture(int width, int height, NVPixelFormat format) {
    return std::make_unique<MetalTexture>(this, width, height, format);
}

std::unique_ptr<NVPipeline> MetalDevice::createRenderPipeline(const NVPipelineDesc& desc) {
    return MetalPipeline::createRender(this, desc);
}

std::unique_ptr<NVPipeline> MetalDevice::createComputePipeline(const NVComputePipelineDesc& desc) {
    return MetalPipeline::createCompute(this, desc);
}

void MetalDevice::submit(NVCommandBuffer* cmd) {
    auto* metalCmd = static_cast<MetalCommandBuffer*>(cmd);
    metalCmd->commit();
}

void MetalDevice::waitIdle() {
    @autoreleasepool {
        auto* queue = (__bridge id<MTLCommandQueue>)m_commandQueue;
        id<MTLCommandBuffer> buf = [queue commandBuffer];
        [buf commit];
        [buf waitUntilCompleted];
    }
}

std::string MetalDevice::deviceName() const {
    @autoreleasepool {
        auto* device = (__bridge id<MTLDevice>)m_device;
        return std::string([[device name] UTF8String]);
    }
}

size_t MetalDevice::maxBufferSize() const {
    @autoreleasepool {
        auto* device = (__bridge id<MTLDevice>)m_device;
        return static_cast<size_t>([device maxBufferLength]);
    }
}

} // namespace nv
