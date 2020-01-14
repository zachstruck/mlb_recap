#include "shader.hpp"

#include "file_utility.hpp"

#include <filesystem>

Mlb::Shader::Shader(
    std::filesystem::path const& vertexFilename,
    std::filesystem::path const& fragmentFilename)
{
    // Read the shader programs from the filesystem
    std::string const vertexCode = readTextFile(vertexFilename);
    std::string const fragmentCode = readTextFile(fragmentFilename);

    // Build and compile shader program

    auto const checkShaderCompile = [](GLuint shader)
    {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (success == GL_FALSE)
        {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            std::string infoLog(logLength, '\0');
            glGetShaderInfoLog(shader, static_cast<GLsizei>(infoLog.size()), nullptr, infoLog.data());

            throw std::runtime_error(infoLog);
        }
    };

    auto const checkShaderLink = [](GLuint program)
    {
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (success == GL_FALSE)
        {
            GLint logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

            std::string infoLog(logLength, '\0');
            glGetShaderInfoLog(program, static_cast<GLsizei>(infoLog.size()), nullptr, infoLog.data());

            throw std::runtime_error(infoLog);
        }
    };

    // Vertex shader
    GLuint const vertexShader = glCreateShader(GL_VERTEX_SHADER);
    char const* const vertexShaderSource = vertexCode.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    checkShaderCompile(vertexShader);

    // Fragment shader
    GLuint const fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    char const* const fragmentShaderSource = fragmentCode.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkShaderCompile(fragmentShader);

    // Link shaders
    shaderProgramId_ = glCreateProgram();
    glAttachShader(shaderProgramId_, vertexShader);
    glAttachShader(shaderProgramId_, fragmentShader);
    glLinkProgram(shaderProgramId_);
    checkShaderLink(shaderProgramId_);

    // This needs RAII
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

GLuint Mlb::Shader::id() const noexcept
{
    return shaderProgramId_;
}

void Mlb::Shader::use() const noexcept
{
    glUseProgram(shaderProgramId_);
}
