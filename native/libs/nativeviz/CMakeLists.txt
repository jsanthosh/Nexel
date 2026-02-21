cmake_minimum_required(VERSION 3.24)
project(nativeviz
    VERSION 0.1.0
    LANGUAGES CXX OBJCXX
    DESCRIPTION "High-performance GPU-accelerated data visualization library")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 20)

# --- Source files ---

set(NV_HAL_SOURCES
    src/hal/metal/MetalDevice.mm
    src/hal/metal/MetalCommandBuffer.mm
    src/hal/metal/MetalBuffer.mm
    src/hal/metal/MetalTexture.mm
    src/hal/metal/MetalPipeline.mm
)

set(NV_RENDER_SOURCES
    src/render/NVRenderer2D.cpp
    src/render/NVPath.cpp
    src/render/NVGradient.cpp
)

set(NV_DATA_SOURCES
    src/data/NVDataStore.cpp
    src/data/NVDecimator.cpp
    src/data/NVDataUploader.cpp
)

set(NV_CHART_SOURCES
    src/chart/NVTheme.cpp
    src/chart/NVAxis.cpp
    src/chart/NVLegend.cpp
    src/chart/NVChart.cpp
    src/chart/types/NVLineChart.cpp
    src/chart/types/NVBarChart.cpp
    src/chart/types/NVScatterChart.cpp
)

set(NV_SCENE_SOURCES
    src/scene/NVScene.cpp
)

set(NV_API_SOURCES
    src/api/nv_api.cpp
)

# --- Core library (no Qt dependency) ---

add_library(nativeviz STATIC
    ${NV_HAL_SOURCES}
    ${NV_RENDER_SOURCES}
    ${NV_DATA_SOURCES}
    ${NV_CHART_SOURCES}
    ${NV_SCENE_SOURCES}
    ${NV_API_SOURCES}
)

target_include_directories(nativeviz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hal
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/metal
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render
        ${CMAKE_CURRENT_SOURCE_DIR}/src/data
        ${CMAKE_CURRENT_SOURCE_DIR}/src/chart
        ${CMAKE_CURRENT_SOURCE_DIR}/src/chart/types
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene
        ${CMAKE_CURRENT_SOURCE_DIR}/src/api
)

# macOS frameworks
if(APPLE)
    target_link_libraries(nativeviz
        PRIVATE
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
            "-framework Foundation"
    )
    # Force native architecture
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}")
endif()

target_compile_options(nativeviz PRIVATE -Wall -Wextra -Wno-unused-private-field -O2)

# Enable ARC for Objective-C++ files
set_source_files_properties(${NV_HAL_SOURCES} PROPERTIES
    COMPILE_FLAGS "-fobjc-arc"
)

# Metal shaders are compiled at runtime from embedded source strings
# (see src/hal/metal/MetalShaderSource.h)
# When full Xcode is available, offline compilation can be enabled for
# faster startup by uncommenting the block below and setting NV_USE_PRECOMPILED_SHADERS
# target_compile_definitions(nativeviz PRIVATE NV_USE_PRECOMPILED_SHADERS)
