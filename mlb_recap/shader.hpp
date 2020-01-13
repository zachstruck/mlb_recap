#ifndef MLB_SHADER_HPP
#define MLB_SHADER_HPP

#include <glad/glad.h>

#include <filesystem>

namespace Mlb
{
    class Shader
    {
    public:
        explicit Shader(
            std::filesystem::path const& vertexFilename,
            std::filesystem::path const& fragmentFilename);

        GLuint id() const noexcept;

        void use() const noexcept;

    private:
        GLuint shaderProgramId_{};
    };
}

#endif
