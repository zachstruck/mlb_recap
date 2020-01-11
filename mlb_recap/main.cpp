#include <curl/curl.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

    void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

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

    std::string loadFromInternet(std::string const& url)
    {
        std::string data;

        CurlInit curl;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
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

    std::string loadFromLocal(std::filesystem::path const& filename)
    {
        std::ifstream file(filename, std::ios::in | std::ios::binary);

        if (!file)
        {
            throw std::runtime_error("Failed to open file");
        }

        std::string data;

        file.seekg(0, std::ios::end);
        data.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(data.data(), data.size());

        return data;
    }

    std::string getMlbFeed()
    {
#if 0
        constexpr const char* const url = "http://statsapi.mlb.com/api/v1/schedule?hydrate=game(content(editorial(recap))),decisions&date=2018-06-10&sportId=1";
        return loadFromInternet(url);
#else
        std::filesystem::path const filename = "../res/schedule_2018-06-10.json";
        return loadFromLocal(filename);
#endif
    }
}

namespace
{
    class GameData final
    {
    public:
        GameData() = default;

        template <typename P1, typename P2, typename P3>
        GameData(P1&& headline, P2&& subhead, P3&& photoUrl) :
            headline(std::forward<P1>(headline)),
            subhead(std::forward<P1>(subhead)),
            photoUrl(std::forward<P1>(photoUrl))
        {
        }

        std::string headline;
        std::string subhead;
        std::string photoUrl;
    };

    using MlbData = std::vector<GameData>;

    MlbData parseMlb()
    {
        auto const json = nlohmann::json::parse(getMlbFeed());

        MlbData mlbData;

        auto const& jsonGames = json["dates"][0]["games"];

        mlbData.reserve(jsonGames.size());
        for (auto const& game : jsonGames)
        {
            auto const& jsonMlb = game["content"]["editorial"]["recap"]["mlb"];

            jsonMlb["photo"]["cuts"]["270x154"]["src"];

            mlbData.emplace_back(
                jsonMlb["headline"].get<std::string>(),
                jsonMlb["subhead"].get<std::string>(),
                // Arbitrarily choosing a size here
                jsonMlb["photo"]["cuts"]["270x154"]["src"].get<std::string>());
        }

        return mlbData;
    }
}

int main()
{
    try
    {
        MlbData const mblData = parseMlb();

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
