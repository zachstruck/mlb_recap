#include "font_utility.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <filesystem>

namespace
{
    class FreeTypeInit final
    {
    public:
        FreeTypeInit()
        {
            if (FT_Init_FreeType(&ft_))
            {
                throw std::runtime_error("Failed to initialize freetype library");
            }
        }

        // Disable copy semantics
        FreeTypeInit(FreeTypeInit const& rhs) = delete;
        FreeTypeInit& operator=(FreeTypeInit const& rhs) = delete;

        // Disable move semantics
        FreeTypeInit(FreeTypeInit&& rhs) noexcept = delete;
        FreeTypeInit& operator=(FreeTypeInit&& rhs) noexcept = delete;

        ~FreeTypeInit()
        {
            FT_Done_FreeType(ft_);
        }

        FT_Library const& get() const noexcept
        {
            return ft_;
        }

    private:
        FT_Library ft_{};
    };
}

Mlb::CharacterSet Mlb::loadCharacterSet(std::filesystem::path const& filename)
{
    // FIXME
    // Need to move this initialization of this function
    // to avoid redoing work for every load
    FreeTypeInit const freeType;

    FT_Face face;
    if (FT_New_Face(freeType.get(), filename.string().c_str(), 0, &face))
    {
        throw std::runtime_error("Failed to load font: " + filename.string());
    }

    // Set the font size
    FT_Set_Pixel_Sizes(face, 0, 24);

    std::array<Character, 128> characters;

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (GLubyte c = 0; c < 128; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            characters[c] = {};
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Cache the glyph
        characters[c] = Character{
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x),
        };
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // This needs RAII
    FT_Done_Face(face);

    return characters;
}
