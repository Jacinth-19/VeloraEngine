#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace velora {

// Orbital camera: looks at `target` from a spherical orbit defined by
// distance / elevation / azimuth.
struct Camera {
  glm::vec3 target{0.0f, 0.8f, 0.0f};
  float distance = 9.0f;
  float elevationDeg = 20.0f;
  float azimuthDeg = 30.0f;
  float fovDeg = 45.0f;
  float nearPlane = 0.1f;
  float farPlane = 100.0f;

  glm::vec3 eye() const {
    float el = glm::radians(elevationDeg);
    float az = glm::radians(azimuthDeg);
    return target + distance * glm::vec3(glm::cos(el) * glm::sin(az), glm::sin(el),
                                         glm::cos(el) * glm::cos(az));
  }

  glm::mat4 view() const { return glm::lookAt(eye(), target, glm::vec3(0, 1, 0)); }

  glm::mat4 proj(float aspect) const {
    glm::mat4 p = glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);
    p[1][1] *= -1.0f; // GL -> Vulkan clip space
    return p;
  }
};

} // namespace velora
