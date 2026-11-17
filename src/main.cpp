// Velora Engine — an NPR-first game engine (C++20 / Vulkan).
// Entry point + command-line interface.

#include "core/Log.hpp"
#include "render/Renderer.hpp"
#include "render/Scene.hpp"
#include "vk/ShaderLibrary.hpp"
#include "vk/VulkanContext.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

const char* kBanner = R"(
  _   __     __                    ____            _
 | | / /__  / /__  _______ _     / __/__  ___ _  (_)__  ___
 | |/ / -_) / _ \/ __/ _ `/    / _// _ \/ _ `/ / / _ \/ -_)
 |___/\__/_/\___/_/  \_,_/    /___/_//_/\_, /_/_/_//_/\__/
     paint-first rendering             /___/      v0.1.0
)";

struct Options {
  std::string command = "showcase";
  std::string scene = "gallery";
  std::string style = "toon";
  std::string out = "renders";
  uint32_t width = 960;
  uint32_t height = 720;
  float angle = 0.0f;
  bool angleSet = false;
  int ssaa = 2;
  std::string shaderDir = "shaders";
};

void printHelp() {
  std::puts(kBanner);
  std::puts(
      "Velora Engine — non-photorealistic rendering game engine (C++20 / Vulkan)\n"
      "\n"
      "Usage: velora [command] [options]\n"
      "\n"
      "Commands:\n"
      "  showcase            Render every scene x style matrix plus a turntable (default)\n"
      "  render              Render a single frame (--scene/--style/--angle/--out)\n"
      "  list                List available scenes, styles and meshes\n"
      "  info                Print Vulkan device information\n"
      "  help                Show this help\n"
      "\n"
      "Options:\n"
      "  --scene NAME        gallery | knot | solar          (default: gallery)\n"
      "  --style NAME        toon | gooch | hatch | halftone | ink (default: toon)\n"
      "  --angle DEG         Camera azimuth in degrees        (default: scene's own)\n"
      "  --out PATH          Output PNG (render) or directory (showcase; default renders/)\n"
      "  --size WxH          Output resolution                (default: 960x720)\n"
      "  --ssaa N            Supersampling factor             (default: 2)\n"
      "  --shaders DIR       GLSL shader directory            (default: shaders)\n"
      "\n"
      "Environment:\n"
      "  VK_ICD_FILENAMES    Path to a Vulkan ICD manifest (e.g. SwiftShader for\n"
      "                      headless machines without a GPU).\n");
}

void printList() {
  std::puts("Scenes:");
  for (const auto& s : velora::sceneNames())
    std::printf("  %s\n", s.c_str());
  std::puts("Styles:");
  for (const auto& s : velora::styleList())
    std::printf("  %-9s %s\n", s.name.c_str(), s.description.c_str());
  std::puts("Meshes: sphere, cube, cone, knot, crystal, plane");
}

Options parseArgs(int argc, char** argv) {
  Options o;
  bool commandSeen = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&](const char* flag) -> std::string {
      if (i + 1 >= argc)
        velora::log::fatal(std::string("missing value for ") + flag);
      return argv[++i];
    };
    if (a == "--scene") o.scene = next("--scene");
    else if (a == "--style") o.style = next("--style");
    else if (a == "--out") o.out = next("--out");
    else if (a == "--angle") { o.angle = std::stof(next("--angle")); o.angleSet = true; }
    else if (a == "--ssaa") o.ssaa = std::stoi(next("--ssaa"));
    else if (a == "--shaders") o.shaderDir = next("--shaders");
    else if (a == "--size") {
      std::string s = next("--size");
      if (std::sscanf(s.c_str(), "%ux%u", &o.width, &o.height) != 2)
        velora::log::fatal("bad --size (expected WxH, e.g. 1280x720)");
    } else if (!commandSeen && a[0] != '-') {
      o.command = a;
      commandSeen = true;
    } else {
      velora::log::fatal("unknown argument: " + a);
    }
  }
  return o;
}

} // namespace

int main(int argc, char** argv) {
  Options opts = parseArgs(argc, argv);

  if (opts.command == "help" || opts.command == "--help" || opts.command == "-h") {
    printHelp();
    return 0;
  }
  if (opts.command == "list") {
    printList();
    return 0;
  }

  velora::log::info("Velora Engine v0.1.0 — NPR renderer starting");
  velora::initShaderCompiler();

  velora::VulkanContext ctx;
  ctx.init();

  if (opts.command == "info") {
    std::printf("Device:              %s\n", ctx.props.deviceName);
    std::printf("Vulkan API:          %u.%u.%u\n", VK_VERSION_MAJOR(ctx.props.apiVersion),
                VK_VERSION_MINOR(ctx.props.apiVersion),
                VK_VERSION_PATCH(ctx.props.apiVersion));
    std::printf("Max image 2D:        %u\n", ctx.props.limits.maxImageDimension2D);
    std::printf("Max push constants:  %u bytes\n", ctx.props.limits.maxPushConstantsSize);
    std::printf("Memory heaps:        %u\n", ctx.memProps.memoryHeapCount);
    ctx.shutdown();
    velora::shutdownShaderCompiler();
    return 0;
  }

  {
    velora::Renderer renderer(ctx, opts.width, opts.height, opts.ssaa, opts.shaderDir);

    if (opts.command == "render") {
      velora::Scene scene = velora::makeScene(opts.scene);
      std::string out = opts.out;
      if (out == "renders") // default: build a filename under renders/
        out = "renders/" + opts.scene + "_" + opts.style + ".png";
      float offset = opts.angleSet ? opts.angle - scene.camera.azimuthDeg : 0.0f;
      renderer.render(scene, opts.style, offset, out);
    } else if (opts.command == "showcase") {
      const std::string dir = opts.out;
      int count = 0;
      for (const auto& sceneName : velora::sceneNames()) {
        velora::Scene scene = velora::makeScene(sceneName);
        for (const auto& style : velora::styleList()) {
          renderer.render(scene, style.name, 0.0f,
                          dir + "/" + sceneName + "_" + style.name + ".png");
          count++;
        }
      }
      // Turntable of the hero knot in toon style.
      velora::Scene knot = velora::makeScene("knot");
      for (int i = 0; i < 8; i++) {
        char name[128];
        std::snprintf(name, sizeof(name), "%s/turntable_knot_%03d.png", dir.c_str(),
                      i * 45);
        renderer.render(knot, "toon", (float)(i * 45), name);
        count++;
      }
      std::printf("showcase complete: %d renders written to %s/\n", count, dir.c_str());
    } else {
      velora::log::fatal("unknown command: " + opts.command + " (try 'velora help')");
    }
  } // renderer destroyed before the device

  ctx.shutdown();
  velora::shutdownShaderCompiler();
  return 0;
}
