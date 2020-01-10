#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

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
}

int main()
{
    try
    {
        /* Initialize the library */
        GlfwInit const glfw;

        /* Create a windowed mode window and its OpenGL context */
        GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", nullptr, nullptr);
        if (window == nullptr)
        {
            throw std::runtime_error("Failed to create a window");
        }

        /* Make the window's context current */
        glfwMakeContextCurrent(window);

        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            glClear(GL_COLOR_BUFFER_BIT);

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }
    catch (std::exception const& ex)
    {
        std::printf("Error: %s\n", ex.what());
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
