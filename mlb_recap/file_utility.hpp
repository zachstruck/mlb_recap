#ifndef MLB_FILE_UTILITY_HPP
#define MLB_FILE_UTILITY_HPP

#include <filesystem>
#include <string>

namespace Mlb
{
    std::string readTextFile(std::filesystem::path const& filename);
}

#endif
