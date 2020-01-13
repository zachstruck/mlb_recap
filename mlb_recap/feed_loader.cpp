#include "feed_loader.hpp"

#include "file_utility.hpp"

#include <curl/curl.h>

#include <nlohmann/json.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

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
        auto& data = *static_cast<std::vector<std::byte>*>(userp);

        std::size_t const realsize = size * nmemb;

        auto const* const contents_ = static_cast<std::byte*>(contents);

        data.insert(
            data.cend(),
            contents_, contents_ + realsize);

        return realsize;
    };

    std::vector<std::byte> downloadFile(CurlInit& curl, std::string const& url)
    {
        std::vector<std::byte> data;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &data);

        // FIXME  Security
        // Occasionally failing with error `CURLE_SSL_CONNECT_ERROR`
        // Disable SSL verification for now
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode const code = curl_easy_perform(curl.get());

        if (code != CURLE_OK)
        {
            throw std::runtime_error("Failed to get URL: " + url);
        }

        return data;
    }

    std::string getMlbFeed(CurlInit& curl)
    {
        constexpr const char* const url = "http://statsapi.mlb.com/api/v1/schedule?hydrate=game(content(editorial(recap))),decisions&date=2018-06-10&sportId=1";

        std::vector<std::byte> const data = downloadFile(curl, url);

        // Seemingly unable to move data around here
        // Need to redesign the API to avoid this copy
        std::string feed(data.size(), '\0');
        std::transform(
            data.begin(), data.end(),
            feed.begin(),
            [](auto const& ch) { return static_cast<char>(ch); });
        return feed;
    }
}

Mlb::MlbData Mlb::getFeedData()
{
    // Initialize libcurl once for loading everything from the internet feed
    CurlInit curl;

    auto const json = nlohmann::json::parse(getMlbFeed(curl));

    MlbData mlbData;

    auto const& jsonGames = json["dates"][0]["games"];

    mlbData.reserve(jsonGames.size());
    for (auto const& game : jsonGames)
    {
        auto const& jsonMlb = game["content"]["editorial"]["recap"]["mlb"];

        mlbData.emplace_back(
            jsonMlb["headline"].get<std::string>(),
            jsonMlb["subhead"].get<std::string>(),
            // Arbitrarily choosing a size here
            downloadFile(curl, jsonMlb["photo"]["cuts"]["270x154"]["src"].get<std::string>()));
    }

    return mlbData;
}
