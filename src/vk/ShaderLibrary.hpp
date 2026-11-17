#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace velora {

// Runtime GLSL -> SPIR-V compilation backed by glslang. Shaders ship as plain
// .vert/.frag files next to the engine and are compiled when pipelines are
// built, so they can be edited and hot-reloaded without a separate toolchain.
void initShaderCompiler();
void shutdownShaderCompiler();

std::vector<uint32_t> compileShaderFile(VkShaderStageFlagBits stage,
                                        const std::string& path);

} // namespace velora
