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

int main() {

  Window window("Renderer");

  jstl::opengl::Shader shader("shader.vert", "shader.frag");
  jstl::opengl::Shader computeShader("shader.comp");

  GLuint framebufferTexture;

  window.setClearColor(glm::vec4(0, 0, 0, 1));

  glDisable(GL_CULL_FACE);
 
  FullScreenQuad fullscreenQuad(&shader);

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

  while (!glfwWindowShouldClose(window.window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    computeShader.use();
    computeShader.setVec2("resolution", window.resolution);
    glBindImageTexture(1, framebufferTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(window.resolution.x / 16, window.resolution.y / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    if (Input::isKeyDown(GLFW_KEY_R)) {
      jstl::opengl::Shader::hotReloadAll();
    }
    
    
    fullscreenQuad.draw(framebufferTexture, "outputTexture");
    glfwSwapBuffers(window.window);
    glfwPollEvents();
  }
  glDeleteTextures(1, &framebufferTexture);
}
