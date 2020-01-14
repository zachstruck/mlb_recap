#include "render_text.hpp"

#include "font_utility.hpp"
#include "shader.hpp"

// NOTE
// Functionality heavily borrowed from a tutorial
// https://learnopengl.com/In-Practice/Text-Rendering

void Mlb::renderHeadlineText(
    GLuint vao, GLuint vbo,
    Mlb::CharacterSet const& charSet,
    Mlb::Shader const& shader,
    std::string_view text,
    GLfloat x, // Centered midpoint
    GLfloat y,
    GLfloat scale,
    GLfloat width, // Text width before ellipsis
    glm::vec3 color)
{
    shader.use();
    glUniform3f(glGetUniformLocation(shader.id(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    GLfloat const ellipsesWidth = 3 * ((charSet['.'].advance >> 6)* scale);
    std::size_t indexEnd = text.size();
    // Determine the length
    GLfloat len = 0.0f;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        Mlb::Character ch = charSet[text[i]];

        len += (ch.advance >> 6)* scale;

        if (len > width)
        {
            std::size_t j = i + 1;
            for (/**/; j-- > 0 && len + ellipsesWidth > width; /**/)
            {
                ch = charSet[text[j]];

                len -= (ch.advance >> 6)* scale;
            }

            len += ellipsesWidth;

            // Guard against unsigned overflow
            indexEnd = j <= indexEnd ? j : 0;

            break;
        }
    }

    // Iterate through all characters
    GLfloat x_ = x - (len / 2.0f);
    for (std::size_t i = 0; i < indexEnd; ++i)
    {
        Mlb::Character const& ch = charSet[text[i]];

        GLfloat const xpos = x_ + ch.bearing.x * scale;
        GLfloat const ypos = y - (ch.size.y - ch.bearing.y) * scale;

        GLfloat const w = ch.size.x * scale;
        GLfloat const h = ch.size.y * scale;

        // Update VBO for each character
        GLfloat const vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 },
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureId);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x_ += (ch.advance >> 6)* scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }

    // Check if ellipses need to be appended
    if (indexEnd != text.size())
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            Mlb::Character const& ch = charSet['.'];

            GLfloat const xpos = x_ + ch.bearing.x * scale;
            GLfloat const ypos = y - (ch.size.y - ch.bearing.y) * scale;

            GLfloat const w = ch.size.x * scale;
            GLfloat const h = ch.size.y * scale;

            // Update VBO for each character
            GLfloat const vertices[6][4] = {
                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos,     ypos,       0.0, 1.0 },
                { xpos + w, ypos,       1.0, 1.0 },

                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos + w, ypos,       1.0, 1.0 },
                { xpos + w, ypos + h,   1.0, 0.0 },
            };

            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureId);

            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x_ += (ch.advance >> 6)* scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
        }
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

namespace
{
    bool isWhitespace(char ch) noexcept
    {
        switch (ch)
        {
        case ' ':
            [[fallthrough]];
        case '\f':
            [[fallthrough]];
        case '\n':
            [[fallthrough]];
        case '\r':
            [[fallthrough]];
        case '\t':
            [[fallthrough]];
        case '\v':
            return true;
        default:
            return false;
        }
    }
}

void Mlb::renderSubheadingText(
    GLuint vao, GLuint vbo,
    Mlb::CharacterSet const& charSet,
    Mlb::Shader const& shader,
    std::string_view text,
    GLfloat x, // Centered midpoint
    GLfloat y,
    GLfloat scale,
    GLfloat width, // Text width before wrapping
    glm::vec3 color)
{
    shader.use();
    glUniform3f(glGetUniformLocation(shader.id(), "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);

    // Cache this in `CharacterSet`
    auto const lineHeight = [&charSet]
    {
        auto const tallestChar = *std::max_element(
            charSet.begin(), charSet.end(),
            [](auto const& lhs, auto const& rhs) {
                return lhs.size.y < rhs.size.y;
            });
        return tallestChar.size.y;
    }();

    std::size_t idx = 0;

    while (idx < text.size())
    {
        auto const idxStart = idx;

        // Determine the length
        GLfloat len = 0.0f;
        for (/**/; idx < text.size(); ++idx)
        {
            Mlb::Character ch = charSet[text[idx]];

            len += (ch.advance >> 6)* scale;

            if (len > width)
            {
                idx += 1;
                for (/**/; idx-- > 0 && !isWhitespace(text[idx]); /**/)
                {
                    ch = charSet[text[idx]];

                    len -= (ch.advance >> 6)* scale;
                }

                if (idx > text.size())
                {
                    // Unsure how to handle the case
                    // where even one character cannot be written
                    return;
                }

                break;
            }
        }

        // Iterate through all characters
        std::size_t const indexEnd = idx;
        GLfloat x_ = x - (len / 2.0f);
        for (std::size_t i = idxStart; i < indexEnd; ++i)
        {
            Mlb::Character const& ch = charSet[text[i]];

            GLfloat const xpos = x_ + ch.bearing.x * scale;
            GLfloat const ypos = y - (ch.size.y - ch.bearing.y) * scale;

            GLfloat const w = ch.size.x * scale;
            GLfloat const h = ch.size.y * scale;

            // Update VBO for each character
            GLfloat const vertices[6][4] = {
                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos,     ypos,       0.0, 1.0 },
                { xpos + w, ypos,       1.0, 1.0 },

                { xpos,     ypos + h,   0.0, 0.0 },
                { xpos + w, ypos,       1.0, 1.0 },
                { xpos + w, ypos + h,   1.0, 0.0 },
            };

            // Render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureId);

            // Update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x_ += (ch.advance >> 6)* scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
        }

        y -= lineHeight * 1.05f; // Move down plus a little extra padding
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}