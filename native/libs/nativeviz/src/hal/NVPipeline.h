#ifndef NV_PIPELINE_H
#define NV_PIPELINE_H

#include "nativeviz/nv_types.h"

namespace nv {

class NVPipeline {
public:
    virtual ~NVPipeline() = default;

    virtual bool isCompute() const = 0;
    virtual void* nativeHandle() const = 0;

protected:
    NVPipeline() = default;
};

} // namespace nv

#endif // NV_PIPELINE_H
