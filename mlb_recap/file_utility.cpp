#include "file_utility.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

std::string Mlb::readTextFile(std::filesystem::path const& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("Failed to open file: " + filename.string());
    }

    std::string data;

    file.seekg(0, std::ios::end);
    data.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(data.data(), data.size());

    return data;
}
