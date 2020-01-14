#ifndef MLB_RENDER_TEXT_HPP
#define MLB_RENDER_TEXT_HPP

#include "font_utility.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string_view>

namespace Mlb
{
    void renderHeadlineText(
        GLuint vao, GLuint vbo,
        Mlb::CharacterSet const& charSet,
        Mlb::Shader const& shader,
        std::string_view text,
        GLfloat x, // Centered midpoint
        GLfloat y,
        GLfloat scale,
        GLfloat width, // Text width before ellipsis
        glm::vec3 color);

    void renderSubheadingText(
        GLuint vao, GLuint vbo,
        Mlb::CharacterSet const& charSet,
        Mlb::Shader const& shader,
        std::string_view text,
        GLfloat x, // Centered midpoint
        GLfloat y,
        GLfloat scale,
        GLfloat width, // Text width before wrapping
        glm::vec3 color);
}

#endif
