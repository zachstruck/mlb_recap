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

        operator FT_Library() const noexcept
        {
            return ft_;
        }

    private:
        FT_Library ft_{};
    };

    class FtFace final
    {
    public:
        FtFace(FT_Library library, std::filesystem::path const& filename, FT_Long face_index)
        {
            if (FT_New_Face(library, filename.string().c_str(), 0, &face_))
            {
                throw std::runtime_error("Failed to load font: " + filename.string());
            }
        }

        // Disable copy semantics
        FtFace(FtFace const& rhs) = delete;
        FtFace& operator=(FtFace const& rhs) = delete;

        // Disable move semantics
        FtFace(FtFace&& rhs) noexcept = delete;
        FtFace& operator=(FtFace&& rhs) noexcept = delete;

        ~FtFace()
        {
            FT_Done_Face(face_);
        }

        operator FT_Face() const noexcept
        {
            return face_;
        }

        FT_Face const& get() const noexcept
        {
            return face_;
        }

    private:
        FT_Face face_{};
    };
}

Mlb::CharacterSet Mlb::loadCharacterSet(std::filesystem::path const& filename)
{
    // FIXME
    // Need to move this initialization of this function
    // to avoid redoing work for every load
    FreeTypeInit const freeType;

    FtFace const face(freeType, filename, 0);

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
            face.get()->glyph->bitmap.width,
            face.get()->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face.get()->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Cache the glyph
        characters[c] = Character{
            texture,
            glm::ivec2(face.get()->glyph->bitmap.width, face.get()->glyph->bitmap.rows),
            glm::ivec2(face.get()->glyph->bitmap_left, face.get()->glyph->bitmap_top),
            static_cast<GLuint>(face.get()->glyph->advance.x),
        };
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return characters;
}
