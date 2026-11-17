#pragma once

#include "render/Camera.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace velora {

struct Material {
  glm::vec3 baseColor{0.8f};
  float bands = 3.0f;                  // toon shading band count
  glm::vec3 specColor{1.0f};
  float shininess = 32.0f;
  float rimStrength = 0.35f;
  float rimPower = 3.0f;
  float outlineWidth = 0.02f;          // world-space ink width factor
  glm::vec3 outlineColor{0.08f, 0.07f, 0.10f};
};

struct Node {
  std::string mesh;
  glm::vec3 position{0.0f};
  glm::vec3 rotationAxis{0.0f, 1.0f, 0.0f};
  float rotationDeg = 0.0f;
  float scale = 1.0f;
  Material material{};
};

struct Scene {
  std::string name;
  std::vector<Node> nodes;
  glm::vec3 lightDir = glm::normalize(glm::vec3(-0.55f, -1.0f, -0.35f));
  glm::vec4 clearColor{0.93f, 0.91f, 0.85f, 1.0f}; // paper
  Camera camera;
};

std::vector<std::string> sceneNames();
Scene makeScene(const std::string& name);

} // namespace velora
