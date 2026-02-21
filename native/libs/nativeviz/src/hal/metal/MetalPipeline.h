#ifndef NV_METAL_PIPELINE_H
#define NV_METAL_PIPELINE_H

#include "../NVPipeline.h"
#include "../NVDevice.h"
#include <memory>

namespace nv {

class MetalDevice;

class MetalPipeline : public NVPipeline {
public:
    ~MetalPipeline() override;

    static std::unique_ptr<MetalPipeline> createRender(MetalDevice* device, const NVPipelineDesc& desc);
    static std::unique_ptr<MetalPipeline> createCompute(MetalDevice* device, const NVComputePipelineDesc& desc);

    bool isCompute() const override { return m_isCompute; }
    void* nativeHandle() const override { return m_pipeline; }

private:
    MetalPipeline() = default;
    void* m_pipeline = nullptr; // id<MTLRenderPipelineState> or id<MTLComputePipelineState>
    bool m_isCompute = false;
};

} // namespace nv

#endif // NV_METAL_PIPELINE_H
