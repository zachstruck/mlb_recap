#include "feed_loader.hpp"
#include "font_utility.hpp"
#include "image_loader.hpp"
#include "render_text.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace
{
    constexpr const int defaultWidth = 1920 / 2;
    constexpr const int defaultHeight = 1080 / 2;

    // FIXME  Global state
    Mlb::Shader const* pShaderProjection = nullptr;
}

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
        if (pShaderProjection != nullptr)
        {
            glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(width), 0.0f, static_cast<GLfloat>(height));
            pShaderProjection->use();
            glUniformMatrix4fv(glGetUniformLocation(pShaderProjection->id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        }

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
        GLFWwindow* window = glfwCreateWindow(defaultWidth, defaultHeight, "MLB Recap", nullptr, nullptr);
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

        // Define the viewport dimensions
        glViewport(0, 0, defaultWidth, defaultHeight);

        // Set OpenGL options
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Mlb::Shader const shader(
            "res/shaders/shader.vert",
            "res/shaders/shader.frag");

        Mlb::Shader const shaderFont(
            "res/shaders/font.vert",
            "res/shaders/font.frag");

        glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(defaultWidth), 0.0f, static_cast<GLfloat>(defaultHeight));
        shaderFont.use();
        glUniformMatrix4fv(glGetUniformLocation(shaderFont.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        // Global pointer to this shader for updating the projection matrix
        // whenever the window size changes
        pShaderProjection = &shaderFont;

        // Setup the background image
        GLuint vboBg;
        GLuint vaoBg;
        GLuint eboBg;
        GLuint textureBg;

        {
            Mlb::ImageData const image("res/images/mlb_ballpark.jpg");

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
            Mlb::ImageData const image(mlbData[i].photo);

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

        // Set up font
        GLuint vaoFont;
        GLuint vboFont;

        {
            auto& vao = vaoFont;
            auto& vbo = vboFont;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);

            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
            glBindVertexArray(0);
        }

        Mlb::CharacterSet const characters = Mlb::loadCharacterSet("res/fonts/Roboto-Regular.ttf");

        // Loop until the user closes the window
        while (!glfwWindowShouldClose(window))
        {
            // Render
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

            // Font rendering
            {
                int width, height;
                glfwGetWindowSize(window, &width, &height);

                std::size_t const offset = selectedIndex - lowerViewableIndex;

                float const textX = [&]
                {
                    float const frac = 2.0f / (viewableCount + 1);
                    int const offset = selectedIndex - lowerViewableIndex;

                    float const x = -(1.0f - frac) + offset * frac;

                    return ((x + 1.0f) / 2.0f) * width;
                }();

                float const textWidth = 0.25f * width;

                // White color
                auto const textColor = glm::vec3(1.0f, 1.0f, 1.0f);

                // Render headline text
                {
                    float const textY = 0.625f * height;
                    renderHeadlineText(
                        vaoFont, vboFont,
                        characters,
                        shaderFont,
                        mlbData[selectedIndex].headline,
                        textX, textY,
                        1.0f,
                        textWidth,
                        textColor);
                }

                // Render subheading text
                {
                    float const textY = 0.35f * height;
                    renderSubheadingText(
                        vaoFont, vboFont,
                        characters,
                        shaderFont,
                        mlbData[selectedIndex].subhead,
                        textX, textY,
                        0.8f,
                        textWidth,
                        textColor);
                }
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
        glDeleteBuffers(1, &vboFont);
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
