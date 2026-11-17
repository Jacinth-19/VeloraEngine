#pragma once

#include "vk/Resources.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace velora {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct MeshData {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

struct GpuMesh {
  Buffer vertexBuffer;
  Buffer indexBuffer;
  uint32_t indexCount = 0;
};

// CPU-side procedural mesh generators (unit-sized; scaled by scene nodes).
MeshData makeSphere(int segments = 48, int rings = 24);
MeshData makeCube(float halfExtent = 0.75f);
MeshData makeCone(int segments = 48, float radius = 0.85f, float height = 1.5f);
MeshData makeTorusKnot(int p = 2, int q = 3, float scale = 0.5f, float tube = 0.16f,
                       int segments = 240, int tubeSegments = 24);
MeshData makeCrystal(); // octahedron with crisp flat faces
MeshData makePlane(float halfSize = 9.0f);

// Uploads MeshData to device-local GPU buffers, caching by name.
class MeshStore {
public:
  void init(VulkanContext* ctx) { ctx_ = ctx; }
  void shutdown();
  const GpuMesh& get(const std::string& name);

private:
  GpuMesh upload(const MeshData& data);

  VulkanContext* ctx_ = nullptr;
  std::unordered_map<std::string, GpuMesh> cache_;
};

} // namespace velora
