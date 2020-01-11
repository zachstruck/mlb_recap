#include <curl/curl.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

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

    void processInput(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

namespace
{
    class CurlInit final
    {
    public:
        CurlInit()
        {
            CURLcode const code = curl_global_init(CURL_GLOBAL_ALL);

            if (code != CURLE_OK)
            {
                throw std::runtime_error("Failed to initialize CURL");
            }

            curl_ = curl_easy_init();

            if (curl_ == nullptr)
            {
                throw std::runtime_error("Failed to initialize CURL");
            }
        }

        // Disable copy semantics
        CurlInit(CurlInit const& rhs) = delete;
        CurlInit& operator=(CurlInit const& rhs) = delete;

        // Disable move semantics
        CurlInit(CurlInit&& rhs) noexcept = delete;
        CurlInit& operator=(CurlInit&& rhs) noexcept = delete;

        ~CurlInit()
        {
            curl_easy_cleanup(curl_);

            curl_global_cleanup();
        }

        CURL* get() noexcept
        {
            return curl_;
        }

        CURL const* get() const noexcept
        {
            return curl_;
        }

    private:
        CURL* curl_{};
    };

    std::size_t writeCallback(void* contents, std::size_t size, std::size_t nmemb, void* userp)
    {
        auto& data = *static_cast<std::string*>(userp);

        std::size_t const realsize = size * nmemb;

        auto const* const contents_ = static_cast<char*>(contents);

        data.insert(
            data.cend(),
            contents_, contents_ + realsize);

        return realsize;
    };

    std::string getMlbFeed()
    {
        constexpr const char* const url = "http://statsapi.mlb.com/api/v1/schedule?hydrate=game(content(editorial(recap))),decisions&date=2018-06-10&sportId=1";

        std::string data;

        CurlInit curl;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url);
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &data);

        CURLcode const code = curl_easy_perform(curl.get());

        if (code != CURLE_OK)
        {
            throw std::runtime_error("Failed to get URL");
        }

        return data;
    }
}

int main()
{
    try
    {
        std::string const data = getMlbFeed();

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

        // Do GLAD loader
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // Loop until the user closes the window
        while (!glfwWindowShouldClose(window))
        {
            processInput(window);

            // Render
            glClear(GL_COLOR_BUFFER_BIT);

            // Swap front and back buffers
            glfwSwapBuffers(window);

            // Poll for and process events
            glfwPollEvents();
        }
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
