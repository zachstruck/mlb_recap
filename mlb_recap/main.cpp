#include "feed_loader.hpp"

#include "shader.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb/stb_image.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    // FIXME  Global state
    std::size_t selectedIndex = 0;
    std::size_t maxSelectableIndex{};
    std::size_t lowerViewableIndex{};
    std::size_t upperViewableIndex{};
    constexpr const std::size_t maxViewable = 5;
}

namespace
{
    class GlfwInit final
    {
    public:
        GlfwInit()
        {
            if (glfwInit() == GLFW_FALSE)
            {
                throw std::runtime_error("Failed to initialize GLFW");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        // Disable copy semantics
        GlfwInit(GlfwInit const& rhs) = delete;
        GlfwInit& operator=(GlfwInit const& rhs) = delete;

        // Disable move semantics
        GlfwInit(GlfwInit&& rhs) noexcept = delete;
        GlfwInit& operator=(GlfwInit&& rhs) noexcept = delete;

        ~GlfwInit()
        {
            glfwTerminate();
        }
    };

    void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void processInput(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_RELEASE)
        {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_LEFT:
                if (selectedIndex > 0)
                {
                    --selectedIndex;

                    if (selectedIndex < lowerViewableIndex)
                    {
                        --lowerViewableIndex;
                        --upperViewableIndex;
                    }
                }
                break;
            case GLFW_KEY_RIGHT:
                if (selectedIndex < maxSelectableIndex)
                {
                    ++selectedIndex;

                    if (selectedIndex > upperViewableIndex)
                    {
                        ++lowerViewableIndex;
                        ++upperViewableIndex;
                    }
                }
                break;
            }
        }
    }
}

namespace
{
    class ImageData final
    {
    public:
        ImageData(std::vector<std::byte> const& rawData)
        {
            int nrChannels;
            stbi_set_flip_vertically_on_load(true);
            data_.reset(stbi_load_from_memory(
                reinterpret_cast<stbi_uc const*>(rawData.data()), rawData.size(),
                &width_, &height_, &nrChannels,
                formatType_));

            if (data_ == nullptr)
            {
                throw std::runtime_error("Failed to load image");
            }
        }

        ImageData(std::filesystem::path const& filename)
        {
            int nrChannels;
            stbi_set_flip_vertically_on_load(true);
            data_.reset(stbi_load(
                filename.string().c_str(),
                &width_, &height_, &nrChannels,
                formatType_));

            if (data_ == nullptr)
            {
                throw std::runtime_error("Failed to load image");
            }
        }

        unsigned char const* data() const noexcept
        {
            return data_.get();
        }

        int width() const noexcept
        {
            return width_;
        }

        int height() const noexcept
        {
            return height_;
        }

        auto formatType() const noexcept
        {
            return formatType_;
        }

    private:
        constexpr static const auto formatType_ = STBI_rgb;

        struct Deleter final
        {
            void operator()(stbi_uc* p) const noexcept
            {
                stbi_image_free(p);
            }
        };

        std::unique_ptr<stbi_uc, Deleter> data_;
        int width_{};
        int height_{};
    };
}

int main()
{
    try
    {
        Mlb::MlbData const mlbData = Mlb::getFeedData();

        // FIXME  Global state
        maxSelectableIndex = !mlbData.empty() ?
            mlbData.size() - 1 :
            0;
        lowerViewableIndex = 0;
        upperViewableIndex = std::min(maxSelectableIndex, maxViewable - 1);

        // Initialize GLFW in this scope
        GlfwInit const glfw;

        // Create a windowed mode window and its OpenGL context
        GLFWwindow* window = glfwCreateWindow(640, 480, "MLB Recap", nullptr, nullptr);
        if (window == nullptr)
        {
            throw std::runtime_error("Failed to create a window");
        }

        // Make the window's context current
        glfwMakeContextCurrent(window);

        // Resize the viewport as necessary
        glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);

        // Register a callback for keypresses
        glfwSetKeyCallback(window, processInput);

        // Do GLAD loader
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        Mlb::Shader const shader(
            "../res/shaders/shader.vert",
            "../res/shaders/shader.frag");

        // Setup the background image
        GLuint vboBg;
        GLuint vaoBg;
        GLuint eboBg;
        GLuint textureBg;

        {
            ImageData const image("../res/images/mlb_ballpark.jpg");

            // Cover the entire background
            std::array const vertices = {
                // positions          // texture coords
                +1.0f, +1.0f, 0.0f,   1.0f, 1.0f,   // top right
                +1.0f, -1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
                -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,   // bottom left
                -1.0f, +1.0f, 0.0f,   0.0f, 1.0f,   // top left
            };
            constexpr std::array<GLuint, 6> const indices[] = {
                0, 1, 3, // first triangle
                1, 2, 3, // second triangle
            };

            auto& vbo = vboBg;
            auto& vao = vaoBg;
            auto& ebo = eboBg;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices->data(), GL_STATIC_DRAW);

            // Position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0 * sizeof(float)));
            glEnableVertexAttribArray(0);
            // Texture
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // Load texture
            auto& texture = textureBg;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Magically fixes the image
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // Set up all the photo cuts
        GLuint vboPhoto;
        GLuint vaoPhoto;
        GLuint eboPhoto;

        {
            // Some vertex data
            std::array const vertices = {
                // positions          // texture coords
                +0.1f, +0.1f, 0.0f,   1.0f, 1.0f,   // top right
                +0.1f, -0.1f, 0.0f,   1.0f, 0.0f,   // bottom right
                -0.1f, -0.1f, 0.0f,   0.0f, 0.0f,   // bottom left
                -0.1f, +0.1f, 0.0f,   0.0f, 1.0f,   // top left
            };
            constexpr std::array<GLuint, 6> const indices[] = {
                0, 1, 3, // first triangle
                1, 2, 3, // second triangle
            };

            auto& vbo = vboPhoto;
            auto& vao = vaoPhoto;
            auto& ebo = eboPhoto;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices->data(), GL_STATIC_DRAW);

            // Position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0 * sizeof(float)));
            glEnableVertexAttribArray(0);
            // Texture
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        std::vector<GLuint> textures(mlbData.size(), 0);

        for (std::size_t i = 0; i < mlbData.size(); ++i)
        {
            ImageData const image(mlbData[i].photo);

            // Load texture
            auto& texture = textures[i];
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Magically fixes the image
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        // Loop until the user closes the window
        while (!glfwWindowShouldClose(window))
        {
            // Render
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw
            shader.use();
            // Background
            {
                glm::mat4 trans = glm::mat4(1.0f);
                GLuint const transformLoc = glGetUniformLocation(shader.id(), "transform");
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

                glBindTexture(GL_TEXTURE_2D, textureBg);
                glBindVertexArray(vaoBg);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
            // Photo cuts
            std::size_t const viewableCount = upperViewableIndex - lowerViewableIndex + 1;
            assert(viewableCount <= maxViewable);
            std::size_t const lower = lowerViewableIndex > 0 ? (lowerViewableIndex - 1) : 0;
            std::size_t const upper = upperViewableIndex < (maxSelectableIndex - 1) ? (upperViewableIndex + 1) : upperViewableIndex;
            for (std::size_t i = lower; i <= upper; ++i)
            {
                float const x_trans = [&]
                {
                    float const frac = 2.0f / (viewableCount + 1);
                    int const offset = i - lowerViewableIndex;

                    return -(1.0f - frac) + offset * frac;
                }();

                glm::mat4 xfm = glm::mat4(1.0f);
                xfm = glm::translate(xfm, glm::vec3(x_trans, 0.0f, 0.0f));
                if (i == selectedIndex)
                {
                    xfm = glm::scale(xfm, glm::vec3(1.5f, 1.5f, 1.0f));
                }

                GLuint const transformLoc = glGetUniformLocation(shader.id(), "transform");
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(xfm));

                glBindTexture(GL_TEXTURE_2D, textures[i]);
                glBindVertexArray(vaoPhoto);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }

            // Swap front and back buffers
            glfwSwapBuffers(window);

            // Poll for and process events
            glfwPollEvents();
        }

        // This needs RAII
        glDeleteVertexArrays(1, &vaoBg);
        glDeleteBuffers(1, &vboBg);
        glDeleteBuffers(1, &eboBg);
        glDeleteTextures(1, &textureBg);
        glDeleteVertexArrays(1, &vaoPhoto);
        glDeleteBuffers(1, &vboPhoto);
        glDeleteBuffers(1, &eboPhoto);
        glDeleteTextures(textures.size(), textures.data());
    }
    catch (std::exception const& ex)
    {
        std::printf("Error: %s\n", ex.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
