#pragma once
// clang-format off
#include <GL/glew.h>
#include <GL/gl.h>
// clang-format on

#include "freetype/freetype.h"
#include <GLFW/glfw3.h>
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

#include <jstl/opengl/shader.hpp>

using namespace jstl::opengl;

namespace jstl::opengl::font {

struct Character {
  GLuint textureID;
  glm::ivec2 size;
  glm::ivec2 bearing;
  FT_Pos advance;
};

struct Font {
  // TODO: let's get rid of this hard coded default path.
  Font(const std::string &path =
           "/usr/share/fonts/truetype/firacode/FiraCode-Bold.ttf") {
    if (FT_Init_FreeType(&ft)) {
      std::cerr << "Could not init FreeType Library" << std::endl;
      return;
    }

    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
      std::cerr << "Failed to load font" << std::endl;
      return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned short c = 0; c < 128; c++) {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
        std::cerr << "Failed to load Glyph" << std::endl;
        continue;
      }

      // Generate texture
      unsigned int texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                   face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                   face->glyph->bitmap.buffer);

      // Set texture options
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      characters[c] = {
          texture,
          glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
          glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
          face->glyph->advance.x};
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    // re-enable allignment requirements.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  }
  static auto defaultFont() -> Font & {
    static Font defaultFont{};
    return defaultFont;
  }
  FT_Library ft;
  FT_Face face;
  std::array<Character, 128> characters;
};

static auto MeasureText(Font &font, const std::string &text, float scale)
    -> glm::vec2 {
  float width = 0.0f;
  float maxHeight = 0.0f;
  float maxBearingY = 0.0f;
  float minBearingY = 0.0f;

  for (const char &c : text) {
    const Character &glyph = font.characters[c];
    width += (glyph.advance >> 6) * scale;

    float glyphHeight = glyph.size.y * scale;
    float bearingY = glyph.bearing.y * scale;

    if (bearingY > maxBearingY) {
      maxBearingY = bearingY;
    }
    if ((bearingY - glyphHeight) < minBearingY) {
      minBearingY = bearingY - glyphHeight;
    }
  }
  float height = maxBearingY - minBearingY;
  return {width, height};
}

struct FontRenderer {
  static constexpr const char *vertexSource = R"DELIM(
  #version 450 core
  layout(location = 0) out vec2 TexCoords;
  layout(location = 0) in vec4 vertex;
  layout(location = 2) uniform mat4 projection;

  void main()
  {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
  }
)DELIM";

  static constexpr const char *fragSource = R"DELIM(
    #version 450 core

    layout(location = 0) in vec2 TexCoords;
    layout(location = 0) out vec4 FragColor;

    layout(location = 0) uniform sampler2D text;
    layout(location = 1) uniform vec3 color;

    void main()
    {
      vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
      FragColor = vec4(color, 1.0) * sampled;
    }
  )DELIM";

  glm::vec2 viewport;
  glm::mat4 projection;
  FontRenderer()
      : shader(Shader::loadFromSource(jstl::opengl::Shader::Kind::Vertex,
                                      vertexSource, fragSource)) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  inline ~FontRenderer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }

  inline auto setViewport(const glm::vec2 viewport) {
    this->projection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y);
    this->viewport = viewport;
  }

  inline auto renderText(const std::string &text, glm::vec2 pos, float scale,
                         const glm::vec3 &color,
                         Font &font = Font::defaultFont()) -> void {
       
    pos.y += (36 * scale);
    shader.use();
    shader.setMat4("projection", projection);
    shader.setVec3("color", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    
    const auto originalPos = pos;
    
    const float lineHeight = scale * 48;
    for (const auto &character : text) {
      if (character == '\n') {
        pos.y += lineHeight;
        pos.x = originalPos.x;
        continue;
      }
      const Character &ch = font.characters[character];
      float xpos = pos.x + ch.bearing.x * scale;
      float ypos = viewport.y - pos.y - (ch.size.y - ch.bearing.y) * scale;
      
      float width = ch.size.x * scale;
      float height = ch.size.y * scale;
      
      float vertices[6][4] = {{xpos, ypos + height, 0.0f, 0.0f},
                              {xpos, ypos, 0.0f, 1.0f},
                              {xpos + width, ypos, 1.0f, 1.0f},

                              {xpos, ypos + height, 0.0f, 0.0f},
                              {xpos + width, ypos, 1.0f, 1.0f},
                              {xpos + width, ypos + height, 1.0f, 0.0f}};
                              
                              
      glBindTexture(GL_TEXTURE_2D, ch.textureID);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      pos.x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

private:
  Shader shader;
  GLuint vao, vbo;
};

} // namespace jstl::opengl::font