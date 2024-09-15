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

using namespace jstl::opengl;

int main() {

  Window window("Renderer");

  Shader shader("shader.vert", "shader.frag");
  Shader computeShader("shader.comp");

  FullScreenQuad fullscreenQuad(&shader);
  GLuint framebufferTexture;

  window.setClearColor(glm::vec4(0, 0, 0, 1));

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

  while (!glfwWindowShouldClose(window.window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    computeShader.use();
    computeShader.setVec2("resolution", window.resolution);
    computeShader.setDVec2("pan", pan);
    computeShader.setFloat("zoom", zoom);

    glBindImageTexture(1, framebufferTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_RGBA32F);
    glDispatchCompute(window.resolution.x / 16, window.resolution.y / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    fullscreenQuad.draw(framebufferTexture, "outputTexture");
    
    glFinish();
    
    glfwSwapBuffers(window.window);
    glfwPollEvents();

    if (Input::isKeyDown(GLFW_KEY_R)) {
      Shader::hotReloadAll();
      pan = {0, 0};
      zoom = 1.0f;
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
  glDeleteTextures(1, &framebufferTexture);
}
