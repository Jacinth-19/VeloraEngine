#include "vk/ShaderLibrary.hpp"

#include "core/Log.hpp"

#include <SPIRV/GlslangToSpv.h>
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

#include <fstream>
#include <sstream>

namespace velora {

// Default resource limits for shader compilation (mirrors glslang's
// StandAlone/ResourceLimits.cpp so we don't depend on that binary target).
static const TBuiltInResource& defaultResources() {
  static const TBuiltInResource res = [] {
    TBuiltInResource r{};
    r.maxLights = 32;
    r.maxClipPlanes = 6;
    r.maxTextureUnits = 32;
    r.maxTextureCoords = 32;
    r.maxVertexAttribs = 64;
    r.maxVertexUniformComponents = 4096;
    r.maxVaryingFloats = 64;
    r.maxVertexTextureImageUnits = 32;
    r.maxCombinedTextureImageUnits = 80;
    r.maxTextureImageUnits = 32;
    r.maxFragmentUniformComponents = 4096;
    r.maxDrawBuffers = 32;
    r.maxVertexUniformVectors = 128;
    r.maxVaryingVectors = 8;
    r.maxFragmentUniformVectors = 16;
    r.maxVertexOutputVectors = 16;
    r.maxFragmentInputVectors = 15;
    r.minProgramTexelOffset = -8;
    r.maxProgramTexelOffset = 7;
    r.maxClipDistances = 8;
    r.maxComputeWorkGroupCountX = 65535;
    r.maxComputeWorkGroupCountY = 65535;
    r.maxComputeWorkGroupCountZ = 65535;
    r.maxComputeWorkGroupSizeX = 1024;
    r.maxComputeWorkGroupSizeY = 1024;
    r.maxComputeWorkGroupSizeZ = 64;
    r.maxComputeUniformComponents = 1024;
    r.maxComputeTextureImageUnits = 16;
    r.maxComputeImageUniforms = 8;
    r.maxComputeAtomicCounters = 8;
    r.maxComputeAtomicCounterBuffers = 1;
    r.maxVaryingComponents = 60;
    r.maxVertexOutputComponents = 64;
    r.maxGeometryInputComponents = 64;
    r.maxGeometryOutputComponents = 128;
    r.maxFragmentInputComponents = 128;
    r.maxImageUnits = 8;
    r.maxCombinedImageUnitsAndFragmentOutputs = 8;
    r.maxCombinedShaderOutputResources = 8;
    r.maxImageSamples = 0;
    r.maxVertexImageUniforms = 0;
    r.maxTessControlImageUniforms = 0;
    r.maxTessEvaluationImageUniforms = 0;
    r.maxGeometryImageUniforms = 0;
    r.maxFragmentImageUniforms = 8;
    r.maxCombinedImageUniforms = 8;
    r.maxGeometryTextureImageUnits = 16;
    r.maxGeometryOutputVertices = 256;
    r.maxGeometryTotalOutputComponents = 1024;
    r.maxGeometryUniformComponents = 1024;
    r.maxGeometryVaryingComponents = 64;
    r.maxTessControlInputComponents = 128;
    r.maxTessControlOutputComponents = 128;
    r.maxTessControlTextureImageUnits = 16;
    r.maxTessControlUniformComponents = 1024;
    r.maxTessControlTotalOutputComponents = 4096;
    r.maxTessEvaluationInputComponents = 128;
    r.maxTessEvaluationOutputComponents = 128;
    r.maxTessEvaluationTextureImageUnits = 16;
    r.maxTessEvaluationUniformComponents = 1024;
    r.maxTessPatchComponents = 120;
    r.maxPatchVertices = 32;
    r.maxTessGenLevel = 64;
    r.maxViewports = 16;
    r.maxVertexAtomicCounters = 0;
    r.maxTessControlAtomicCounters = 0;
    r.maxTessEvaluationAtomicCounters = 0;
    r.maxGeometryAtomicCounters = 0;
    r.maxFragmentAtomicCounters = 8;
    r.maxCombinedAtomicCounters = 8;
    r.maxAtomicCounterBindings = 1;
    r.maxVertexAtomicCounterBuffers = 0;
    r.maxTessControlAtomicCounterBuffers = 0;
    r.maxTessEvaluationAtomicCounterBuffers = 0;
    r.maxGeometryAtomicCounterBuffers = 0;
    r.maxFragmentAtomicCounterBuffers = 1;
    r.maxCombinedAtomicCounterBuffers = 1;
    r.maxAtomicCounterBufferSize = 16384;
    r.maxTransformFeedbackBuffers = 4;
    r.maxTransformFeedbackInterleavedComponents = 64;
    r.maxCullDistances = 8;
    r.maxCombinedClipAndCullDistances = 8;
    r.maxSamples = 4;
    r.maxMeshOutputVerticesNV = 256;
    r.maxMeshOutputPrimitivesNV = 512;
    r.maxMeshWorkGroupSizeX_NV = 32;
    r.maxMeshWorkGroupSizeY_NV = 1;
    r.maxMeshWorkGroupSizeZ_NV = 1;
    r.maxTaskWorkGroupSizeX_NV = 32;
    r.maxTaskWorkGroupSizeY_NV = 1;
    r.maxTaskWorkGroupSizeZ_NV = 1;
    r.maxMeshViewCountNV = 4;
    r.limits.nonInductiveForLoops = true;
    r.limits.whileLoops = true;
    r.limits.doWhileLoops = true;
    r.limits.generalUniformIndexing = true;
    r.limits.generalAttributeMatrixVectorIndexing = true;
    r.limits.generalVaryingIndexing = true;
    r.limits.generalSamplerIndexing = true;
    r.limits.generalVariableIndexing = true;
    r.limits.generalConstantMatrixVectorIndexing = true;
    return r;
  }();
  return res;
}

void initShaderCompiler() {
  if (!glslang::InitializeProcess())
    log::fatal("glslang initialization failed");
}

void shutdownShaderCompiler() { glslang::FinalizeProcess(); }

std::vector<uint32_t> compileShaderFile(VkShaderStageFlagBits stage,
                                        const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    log::fatal("cannot open shader file: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string source = ss.str();

  EShLanguage lang = (stage == VK_SHADER_STAGE_VERTEX_BIT) ? EShLangVertex : EShLangFragment;

  glslang::TShader shader(lang);
  const char* strings[1] = {source.c_str()};
  shader.setStrings(strings, 1);
  shader.setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100);
  shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
  shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

  if (!shader.parse(&defaultResources(), 450, false, messages)) {
    log::error("shader parse failed: %s\n%s", path.c_str(), shader.getInfoLog());
    log::fatal("aborting");
  }

  glslang::TProgram program;
  program.addShader(&shader);
  if (!program.link(messages)) {
    log::error("shader link failed: %s\n%s", path.c_str(), program.getInfoLog());
    log::fatal("aborting");
  }

  std::vector<uint32_t> spirv;
  glslang::GlslangToSpv(*program.getIntermediate(lang), spirv);
  log::debug("compiled %s (%zu SPIR-V words)", path.c_str(), spirv.size());
  return spirv;
}

} // namespace velora
