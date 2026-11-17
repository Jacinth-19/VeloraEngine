#include "render/Scene.hpp"

#include "core/Log.hpp"

namespace velora {

namespace palette {
const glm::vec3 coral{0.91f, 0.36f, 0.32f};
const glm::vec3 teal{0.20f, 0.62f, 0.60f};
const glm::vec3 gold{0.96f, 0.72f, 0.24f};
const glm::vec3 sky{0.38f, 0.62f, 0.90f};
const glm::vec3 mint{0.55f, 0.84f, 0.57f};
const glm::vec3 lavender{0.66f, 0.55f, 0.86f};
const glm::vec3 plum{0.55f, 0.30f, 0.56f};
const glm::vec3 paper{0.93f, 0.91f, 0.85f};
const glm::vec3 ink{0.08f, 0.07f, 0.10f};
} // namespace palette

std::vector<std::string> sceneNames() { return {"gallery", "knot", "solar"}; }

// A row of primitives on a ground plane — the NPR "material test scene".
static Scene makeGallery() {
  Scene s;
  s.name = "gallery";
  s.camera.target = {0.0f, 0.9f, 0.0f};
  s.camera.distance = 9.5f;
  s.camera.elevationDeg = 19.0f;
  s.camera.azimuthDeg = 25.0f;

  { // ground
    Node n;
    n.mesh = "plane";
    n.material.baseColor = palette::paper;
    n.material.bands = 2.0f;
    n.material.rimStrength = 0.0f;
    n.material.outlineWidth = 0.0f;
    s.nodes.push_back(n);
  }
  { // coral sphere
    Node n;
    n.mesh = "sphere";
    n.position = {-2.7f, 1.0f, 0.0f};
    n.material.baseColor = palette::coral;
    n.material.bands = 4.0f;
    n.material.shininess = 48.0f;
    s.nodes.push_back(n);
  }
  { // sky cube
    Node n;
    n.mesh = "cube";
    n.position = {-0.9f, 0.75f, 0.35f};
    n.rotationDeg = 32.0f;
    n.material.baseColor = palette::sky;
    n.material.bands = 3.0f;
    s.nodes.push_back(n);
  }
  { // mint cone
    Node n;
    n.mesh = "cone";
    n.position = {0.85f, 0.0f, 0.15f};
    n.material.baseColor = palette::mint;
    n.material.bands = 3.0f;
    n.material.shininess = 24.0f;
    s.nodes.push_back(n);
  }
  { // golden mini knot
    Node n;
    n.mesh = "knot";
    n.position = {2.75f, 1.05f, -0.25f};
    n.rotationAxis = {1.0f, 0.6f, 0.0f};
    n.rotationDeg = 18.0f;
    n.scale = 0.62f;
    n.material.baseColor = palette::gold;
    n.material.bands = 5.0f;
    n.material.shininess = 64.0f;
    n.material.rimStrength = 0.5f;
    s.nodes.push_back(n);
  }
  { // lavender crystal behind
    Node n;
    n.mesh = "crystal";
    n.position = {0.1f, 0.9f, -2.1f};
    n.rotationDeg = 25.0f;
    n.scale = 1.15f;
    n.material.baseColor = palette::lavender;
    n.material.bands = 2.0f;
    n.material.shininess = 90.0f;
    n.material.rimStrength = 0.6f;
    s.nodes.push_back(n);
  }
  return s;
}

// Hero torus knot with satellites.
static Scene makeKnot() {
  Scene s;
  s.name = "knot";
  s.camera.target = {0.0f, 1.2f, 0.0f};
  s.camera.distance = 8.5f;
  s.camera.elevationDeg = 23.0f;
  s.camera.azimuthDeg = 15.0f;

  {
    Node n;
    n.mesh = "plane";
    n.material.baseColor = palette::paper;
    n.material.bands = 2.0f;
    n.material.rimStrength = 0.0f;
    n.material.outlineWidth = 0.0f;
    s.nodes.push_back(n);
  }
  {
    Node n;
    n.mesh = "knot";
    n.position = {0.0f, 1.85f, 0.0f};
    n.rotationAxis = {0.35f, 1.0f, 0.0f};
    n.rotationDeg = -12.0f;
    n.scale = 1.15f;
    n.material.baseColor = palette::gold;
    n.material.bands = 4.0f;
    n.material.shininess = 80.0f;
    n.material.rimStrength = 0.55f;
    n.material.outlineWidth = 0.016f;
    s.nodes.push_back(n);
  }
  const struct { glm::vec3 pos; float scale; glm::vec3 color; float bands; } moons[3] = {
      {{2.45f, 0.55f, 0.55f}, 0.55f, palette::coral, 3.0f},
      {{-2.2f, 0.45f, -0.7f}, 0.45f, palette::teal, 4.0f},
      {{0.5f, 0.4f, 2.35f}, 0.4f, palette::sky, 3.0f},
  };
  for (const auto& moon : moons) {
    Node n;
    n.mesh = "sphere";
    n.position = moon.pos;
    n.scale = moon.scale;
    n.material.baseColor = moon.color;
    n.material.bands = moon.bands;
    n.material.outlineWidth = 0.018f;
    s.nodes.push_back(n);
  }
  return s;
}

// A pocket solar system floating on dark ink paper.
static Scene makeSolar() {
  Scene s;
  s.name = "solar";
  s.clearColor = {0.085f, 0.09f, 0.14f, 1.0f};
  s.lightDir = glm::normalize(glm::vec3(-0.7f, -0.55f, -0.45f));
  s.camera.target = {0.0f, 0.2f, 0.0f};
  s.camera.distance = 12.5f;
  s.camera.elevationDeg = 26.0f;
  s.camera.azimuthDeg = 40.0f;

  { // the sun
    Node n;
    n.mesh = "sphere";
    n.scale = 2.3f;
    n.material.baseColor = {1.0f, 0.72f, 0.22f};
    n.material.bands = 2.0f;
    n.material.rimStrength = 0.8f;
    n.material.rimPower = 2.0f;
    n.material.outlineColor = {0.02f, 0.02f, 0.04f};
    s.nodes.push_back(n);
  }
  const struct { float orbit; float angle; float scale; glm::vec3 color; float y; } planets[5] = {
      {3.4f, 20.0f, 0.34f, palette::teal, 0.1f},
      {4.5f, 100.0f, 0.52f, palette::coral, -0.15f},
      {5.6f, 190.0f, 0.46f, palette::mint, 0.25f},
      {6.9f, 265.0f, 0.72f, palette::sky, -0.3f},
      {8.1f, 330.0f, 0.38f, palette::lavender, 0.15f},
  };
  for (const auto& p : planets) {
    Node n;
    n.mesh = "sphere";
    float a = glm::radians(p.angle);
    n.position = {std::cos(a) * p.orbit, p.y, std::sin(a) * p.orbit};
    n.scale = p.scale;
    n.material.baseColor = p.color;
    n.material.bands = 3.0f;
    n.material.outlineColor = {0.02f, 0.02f, 0.04f};
    s.nodes.push_back(n);
  }
  return s;
}

Scene makeScene(const std::string& name) {
  if (name == "gallery") return makeGallery();
  if (name == "knot") return makeKnot();
  if (name == "solar") return makeSolar();
  log::fatal("unknown scene: " + name);
}

} // namespace velora
