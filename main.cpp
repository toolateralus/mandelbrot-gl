// clang-format off
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <glm/glm.hpp>
#include <jstl/game/utility.hpp>
#include <jstl/opengl/camera.hpp>
#include <jstl/opengl/fullscreen_quad.hpp>
#include <jstl/opengl/mesh.hpp>
#include <jstl/opengl/shader.hpp>
#include <jstl/opengl/window.hpp>

#include "font.hpp"

using namespace jstl::opengl;

int main() {

  Window window("Renderer");

  Shader shader("shader.vert", "shader.frag");
  Shader computeShader("shader.comp");

  font::FontRenderer fontRenderer{};
  fontRenderer.setViewport(window.resolution);

  FullScreenQuad fullscreenQuad(&shader);
  GLuint framebufferTexture;

  window.setClearColor(glm::vec4(0, 0, 0, 1));

  window.resizeEvent().subscribe([&](int x, int y) {
    fontRenderer.setViewport({x, y});
  });

  // Initialize framebuffer texture
  {
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window.resolution.x,
                 window.resolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  window.resizeEvent().subscribe([&](int x, int y) {
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
  });

  glm::dvec2 pan = {0, 0};
  float zoom = 1.0f;
  int samplesPerAxis = 2;

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_BLEND, 0.5f);
  glDisable(GL_DEPTH_TEST);
  glm::vec2 offsets[16];

  window.run([&] {
    const int samples = samplesPerAxis * samplesPerAxis;

    for (int i = 0; i < samplesPerAxis; i++) {
      for (int j = 0; j < samplesPerAxis; j++) {
        offsets[i * samplesPerAxis + j] = glm::vec2(
          float(i) / float(samplesPerAxis), float(j) / float(samplesPerAxis)
        );
      }
    }

    auto transform = glm::dmat4(1.0);
    // apply pan and zoom
    transform = glm::translate(transform, glm::dvec3(pan, 0));
    transform = glm::scale(transform, glm::dvec3(1 / zoom, 1 / zoom, 1));
    // convert screen space to ndc
    transform = glm::translate(transform, glm::dvec3(-1, -1, 0));
    transform = glm::scale(transform, glm::dvec3(2, 2, 1) /
                                          glm::dvec3(window.resolution, 1));

    // render
    {
      computeShader.use();
      computeShader.setVec2("resolution", window.resolution);
      computeShader.setVec2("offsets", offsets[0], 16);
      computeShader.setDMat4("transform", transform);
      computeShader.setInt("maxIterations", 100 * (glm::log(zoom) + 1));
      computeShader.setInt("samples", samples);

      glBindImageTexture(1, framebufferTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                         GL_RGBA32F);
      glDispatchCompute((window.resolution.x + 15) / 16, (window.resolution.y + 15) / 16, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

      static double lastFrameTime = 0;
      double thisFrameTime = glfwGetTime();
      fullscreenQuad.draw(framebufferTexture, "outputTexture");
      fontRenderer.renderText(
          std::format("FPS: {:.1f}", 1 / (thisFrameTime - lastFrameTime)),
          {0, 0}, 1, glm::vec4(1));
      fontRenderer.renderText(
          std::format("MS: {}", samples),
          {0, 48}, 1, glm::vec4(1));
      lastFrameTime = thisFrameTime;
      glFinish();

      // take inputs
      {
        if (Input::isKeyDown(GLFW_KEY_R)) {
          Shader::hotReloadAll();
          pan = {0, 0};
          zoom = 1.0f;
        }

        if (Input::isKeyPressed(GLFW_KEY_UP)) {
          samplesPerAxis = std::min(4, samplesPerAxis + 1);
        }

        if (Input::isKeyPressed(GLFW_KEY_DOWN)) {
          samplesPerAxis = std::max(1, samplesPerAxis - 1);
        }

        static auto lastMousePos = Input::getMousePos();
        float sensitivity = 0.001f;

        if (Input::isButtonDown(GLFW_MOUSE_BUTTON_1)) {
          auto pos = Input::getMousePos();
          auto delta = (lastMousePos - pos) * sensitivity / zoom;
          pan.x += delta.x;
          pan.y -= delta.y;
          lastMousePos = pos;
        } else {
          lastMousePos = Input::getMousePos();
        }

        auto scrollDelta = Input::scrollDelta();

        if (scrollDelta.length() >= 0.1) {
          zoom *= (1.0f + scrollDelta.y * 0.1f);
        }
      }
    }
  });

  glDeleteTextures(1, &framebufferTexture);
}
