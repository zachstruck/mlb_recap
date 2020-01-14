#ifndef MLB_FONT_UTILITY_HPP
#define MLB_FONT_UTILITY_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>
#include <filesystem>

namespace Mlb
{
    struct Character final
    {
        GLuint textureId{};
        glm::ivec2 size{};
        glm::ivec2 bearing{};
        GLuint advance{};
    };

    using CharacterSet = std::array<Character, 128>;

    CharacterSet loadCharacterSet(std::filesystem::path const& filename);
}

#endif
