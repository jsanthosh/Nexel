#include <metal_stdlib>
using namespace metal;

struct DecimationUniforms {
    uint inputCount;     // number of input points
    uint outputCount;    // target number of output points (buckets)
    uint algorithm;      // 0 = min-max, 1 = LTTB
};

// Min-Max decimation: find min and max Y value per bucket
kernel void decimate_minmax(
    constant float* inputX [[buffer(0)]],
    constant float* inputY [[buffer(1)]],
    device float* outputX [[buffer(2)]],
    device float* outputY [[buffer(3)]],
    device uint* outputIndices [[buffer(4)]],
    constant DecimationUniforms& uniforms [[buffer(5)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= uniforms.outputCount) return;

    uint bucketSize = uniforms.inputCount / uniforms.outputCount;
    uint start = gid * bucketSize;
    uint end = min(start + bucketSize, uniforms.inputCount);

    // Each output bucket produces 2 points (min and max)
    uint outIdx = gid * 2;

    float minY = inputY[start];
    float maxY = inputY[start];
    uint minIdx = start;
    uint maxIdx = start;

    for (uint i = start + 1; i < end; i++) {
        if (inputY[i] < minY) {
            minY = inputY[i];
            minIdx = i;
        }
        if (inputY[i] > maxY) {
            maxY = inputY[i];
            maxIdx = i;
        }
    }

    // Output in order of occurrence
    if (minIdx <= maxIdx) {
        outputX[outIdx] = inputX[minIdx];
        outputY[outIdx] = minY;
        outputIndices[outIdx] = minIdx;
        outputX[outIdx + 1] = inputX[maxIdx];
        outputY[outIdx + 1] = maxY;
        outputIndices[outIdx + 1] = maxIdx;
    } else {
        outputX[outIdx] = inputX[maxIdx];
        outputY[outIdx] = maxY;
        outputIndices[outIdx] = maxIdx;
        outputX[outIdx + 1] = inputX[minIdx];
        outputY[outIdx + 1] = minY;
        outputIndices[outIdx + 1] = minIdx;
    }
}

// LTTB (Largest Triangle Three Buckets) - per-bucket kernel
// Each thread computes the selected point for one bucket
kernel void decimate_lttb(
    constant float* inputX [[buffer(0)]],
    constant float* inputY [[buffer(1)]],
    device float* outputX [[buffer(2)]],
    device float* outputY [[buffer(3)]],
    device uint* outputIndices [[buffer(4)]],
    constant DecimationUniforms& uniforms [[buffer(5)]],
    device uint* prevSelected [[buffer(6)]],   // selected index from previous bucket
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= uniforms.outputCount) return;

    // First and last points are always kept
    if (gid == 0) {
        outputX[0] = inputX[0];
        outputY[0] = inputY[0];
        outputIndices[0] = 0;
        prevSelected[0] = 0;
        return;
    }
    if (gid == uniforms.outputCount - 1) {
        uint last = uniforms.inputCount - 1;
        outputX[gid] = inputX[last];
        outputY[gid] = inputY[last];
        outputIndices[gid] = last;
        return;
    }

    uint bucketSize = (uniforms.inputCount - 2) / (uniforms.outputCount - 2);
    uint start = 1 + (gid - 1) * bucketSize;
    uint end = min(start + bucketSize, uniforms.inputCount - 1);

    // Next bucket average (the "third point" in the triangle)
    uint nextStart = end;
    uint nextEnd = min(nextStart + bucketSize, uniforms.inputCount - 1);
    float avgX = 0.0;
    float avgY = 0.0;
    uint nextCount = nextEnd - nextStart;
    for (uint i = nextStart; i < nextEnd; i++) {
        avgX += inputX[i];
        avgY += inputY[i];
    }
    if (nextCount > 0) {
        avgX /= float(nextCount);
        avgY /= float(nextCount);
    }

    // Previous selected point
    uint prevIdx = prevSelected[gid - 1];
    float prevX = inputX[prevIdx];
    float prevY = inputY[prevIdx];

    // Find point in current bucket that maximizes triangle area
    float maxArea = -1.0;
    uint bestIdx = start;

    for (uint i = start; i < end; i++) {
        float area = abs(
            (prevX - avgX) * (inputY[i] - prevY) -
            (prevX - inputX[i]) * (avgY - prevY)
        );
        if (area > maxArea) {
            maxArea = area;
            bestIdx = i;
        }
    }

    outputX[gid] = inputX[bestIdx];
    outputY[gid] = inputY[bestIdx];
    outputIndices[gid] = bestIdx;
    prevSelected[gid] = bestIdx;
}
