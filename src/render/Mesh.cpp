#include "render/Mesh.hpp"

#include "core/Log.hpp"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstring>
#include <functional>

namespace velora {

MeshData makeSphere(int segments, int rings) {
  MeshData m;
  const float pi = glm::pi<float>();
  for (int r = 0; r <= rings; r++) {
    float v = (float)r / (float)rings;
    float phi = v * pi;
    for (int s = 0; s <= segments; s++) {
      float u = (float)s / (float)segments;
      float theta = u * 2.0f * pi;
      glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi),
                  std::sin(phi) * std::sin(theta));
      m.vertices.push_back({n, n});
    }
  }
  for (int r = 0; r < rings; r++) {
    for (int s = 0; s < segments; s++) {
      uint32_t a = r * (segments + 1) + s;
      uint32_t b = a + segments + 1;
      m.indices.insert(m.indices.end(), {a, a + 1, b, b, a + 1, b + 1});
    }
  }
  return m;
}

MeshData makeCube(float h) {
  MeshData m;
  // t x b must equal n for every face (outward, counter-clockwise winding).
  const glm::vec3 n[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  const glm::vec3 t[6] = {{0, 0, -1}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}};
  const glm::vec3 b[6] = {{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}};
  for (int f = 0; f < 6; f++) {
    uint32_t base = (uint32_t)m.vertices.size();
    glm::vec3 c = n[f] * h;
    m.vertices.push_back({c - t[f] * h - b[f] * h, n[f]});
    m.vertices.push_back({c + t[f] * h - b[f] * h, n[f]});
    m.vertices.push_back({c + t[f] * h + b[f] * h, n[f]});
    m.vertices.push_back({c - t[f] * h + b[f] * h, n[f]});
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
  }
  return m;
}

MeshData makeCone(int segments, float radius, float height) {
  MeshData m;
  const float pi = glm::pi<float>();
  // Side: 3 verts per segment (apex + base pair) with smooth radial normals.
  for (int i = 0; i < segments; i++) {
    float a0 = (float)i / segments * 2.0f * pi;
    float a1 = (float)(i + 1) / segments * 2.0f * pi;
    float am = 0.5f * (a0 + a1);
    auto sideNormal = [&](float a) {
      return glm::normalize(glm::vec3(std::cos(a) * height, radius, std::sin(a) * height));
    };
    glm::vec3 b0(std::cos(a0) * radius, 0.0f, std::sin(a0) * radius);
    glm::vec3 b1(std::cos(a1) * radius, 0.0f, std::sin(a1) * radius);
    glm::vec3 apex(0.0f, height, 0.0f);
    uint32_t base = (uint32_t)m.vertices.size();
    m.vertices.push_back({apex, sideNormal(am)});
    m.vertices.push_back({b0, sideNormal(a0)});
    m.vertices.push_back({b1, sideNormal(a1)});
    m.indices.insert(m.indices.end(), {base, base + 2, base + 1});
  }
  // Base disk.
  glm::vec3 down(0.0f, -1.0f, 0.0f);
  uint32_t center = (uint32_t)m.vertices.size();
  m.vertices.push_back({glm::vec3(0.0f), down});
  for (int i = 0; i <= segments; i++) {
    float a = (float)i / segments * 2.0f * pi;
    m.vertices.push_back({glm::vec3(std::cos(a) * radius, 0.0f, std::sin(a) * radius), down});
  }
  for (int i = 0; i < segments; i++)
    m.indices.insert(m.indices.end(), {center, center + 1u + (uint32_t)i,
                                       center + 1u + (uint32_t)(i + 1)});
  return m;
}

MeshData makeTorusKnot(int p, int q, float scale, float tube, int segments,
                       int tubeSegments) {
  MeshData m;
  const float pi = glm::pi<float>();
  const float eps = 1e-3f;

  auto center = [&](float t) {
    float r = 2.0f + std::cos(q * t);
    return scale * glm::vec3(r * std::cos(p * t), std::sin(q * t), r * std::sin(p * t));
  };

  for (int i = 0; i <= segments; i++) {
    float t = (float)i / segments * 2.0f * pi;
    glm::vec3 c = center(t);
    glm::vec3 cPrev = center(t - eps);
    glm::vec3 cNext = center(t + eps);
    glm::vec3 tangent = glm::normalize(cNext - cPrev);
    glm::vec3 curvature = cNext - 2.0f * c + cPrev;
    glm::vec3 n0 = curvature - tangent * glm::dot(curvature, tangent);
    n0 = (glm::length(n0) > 1e-6f) ? glm::normalize(n0) : glm::vec3(0, 1, 0);
    glm::vec3 binormal = glm::normalize(glm::cross(tangent, n0));

    for (int j = 0; j <= tubeSegments; j++) {
      float v = (float)j / tubeSegments * 2.0f * pi;
      glm::vec3 radial = std::cos(v) * n0 + std::sin(v) * binormal;
      m.vertices.push_back({c + radial * tube, radial});
    }
  }

  for (int i = 0; i < segments; i++) {
    for (int j = 0; j < tubeSegments; j++) {
      uint32_t a = i * (tubeSegments + 1) + j;
      uint32_t b = a + tubeSegments + 1;
      m.indices.insert(m.indices.end(), {a, a + 1, b, b, a + 1, b + 1});
    }
  }
  return m;
}

MeshData makeCrystal() {
  // Octahedron with flat faces — reads as a stylized gem/crystal.
  MeshData m;
  const glm::vec3 v[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  const uint32_t f[8][3] = {{2, 4, 0}, {2, 0, 5}, {2, 5, 1}, {2, 1, 4},
                            {3, 0, 4}, {3, 5, 0}, {3, 1, 5}, {3, 4, 1}};
  for (auto& face : f) {
    glm::vec3 a = v[face[0]], b = v[face[1]], c = v[face[2]];
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    uint32_t base = (uint32_t)m.vertices.size();
    m.vertices.push_back({a, n});
    m.vertices.push_back({b, n});
    m.vertices.push_back({c, n});
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2});
  }
  return m;
}

MeshData makePlane(float halfSize) {
  MeshData m;
  glm::vec3 up(0.0f, 1.0f, 0.0f);
  float h = halfSize;
  m.vertices = {{{-h, 0, -h}, up}, {{h, 0, -h}, up}, {{h, 0, h}, up}, {{-h, 0, h}, up}};
  m.indices = {0, 2, 1, 0, 3, 2};
  return m;
}

GpuMesh MeshStore::upload(const MeshData& data) {
  GpuMesh gm;
  gm.indexCount = (uint32_t)data.indices.size();

  VkDeviceSize vbSize = data.vertices.size() * sizeof(Vertex);
  VkDeviceSize ibSize = data.indices.size() * sizeof(uint32_t);

  Buffer vbStaging = createBuffer(*ctx_, vbSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  Buffer ibStaging = createBuffer(*ctx_, ibSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  std::memcpy(mapBuffer(*ctx_, vbStaging), data.vertices.data(), vbSize);
  unmapBuffer(*ctx_, vbStaging);
  std::memcpy(mapBuffer(*ctx_, ibStaging), data.indices.data(), ibSize);
  unmapBuffer(*ctx_, ibStaging);

  gm.vertexBuffer = createBuffer(*ctx_, vbSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  gm.indexBuffer = createBuffer(*ctx_, ibSize,
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  copyBuffer(*ctx_, vbStaging, gm.vertexBuffer, vbSize);
  copyBuffer(*ctx_, ibStaging, gm.indexBuffer, ibSize);

  destroyBuffer(*ctx_, vbStaging);
  destroyBuffer(*ctx_, ibStaging);
  return gm;
}

const GpuMesh& MeshStore::get(const std::string& name) {
  auto it = cache_.find(name);
  if (it != cache_.end())
    return it->second;

  static const std::unordered_map<std::string, std::function<MeshData()>> factories = {
      {"sphere", [] { return makeSphere(); }},
      {"cube", [] { return makeCube(); }},
      {"cone", [] { return makeCone(); }},
      {"knot", [] { return makeTorusKnot(); }},
      {"crystal", [] { return makeCrystal(); }},
      {"plane", [] { return makePlane(); }},
  };
  auto f = factories.find(name);
  if (f == factories.end())
    log::fatal("unknown mesh: " + name);

  auto [emplaced, _] = cache_.emplace(name, upload(f->second()));
  log::info("uploaded mesh '%s' (%u indices)", name.c_str(), emplaced->second.indexCount);
  return emplaced->second;
}

void MeshStore::shutdown() {
  for (auto& [name, gm] : cache_) {
    destroyBuffer(*ctx_, gm.vertexBuffer);
    destroyBuffer(*ctx_, gm.indexBuffer);
  }
  cache_.clear();
}

} // namespace velora
