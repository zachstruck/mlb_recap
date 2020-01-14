#include "image_loader.hpp"

#include <stb/stb_image.h>

#include <utility>
#include <vector>

Mlb::ImageData::ImageData(std::vector<std::byte> const& rawData)
{
    int nrChannels;
    stbi_set_flip_vertically_on_load(true);
    data_.reset(stbi_load_from_memory(
        reinterpret_cast<stbi_uc const*>(rawData.data()),
        static_cast<int>(rawData.size()),
        &width_, &height_, &nrChannels,
        formatType_));

    if (data_ == nullptr)
    {
        throw std::runtime_error("Failed to load image");
    }
}

Mlb::ImageData::ImageData(std::filesystem::path const& filename)
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