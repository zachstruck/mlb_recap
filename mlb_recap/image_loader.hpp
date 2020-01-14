#ifndef MLB_IMAGE_LOADER_HPP
#define MLB_IMAGE_LOADER_HPP

#include <stb/stb_image.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Mlb
{
    class ImageData final
    {
    public:
        ImageData(std::vector<std::byte> const& rawData);

        ImageData(std::filesystem::path const& filename);

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

#endif
