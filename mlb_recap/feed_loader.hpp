#ifndef FEED_LOADER_HPP
#define FEED_LOADER_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Mlb
{
    class GameData final
    {
    public:
        GameData() = default;

        template <typename P1, typename P2, typename P3>
        GameData(P1&& headline, P2&& subhead, P3&& photo) :
            headline(std::forward<P1>(headline)),
            subhead(std::forward<P2>(subhead)),
            photo(std::forward<P3>(photo))
        {
        }

        std::string headline;
        std::string subhead;
        std::vector<std::byte> photo;
    };

    using MlbData = std::vector<GameData>;

    MlbData getFeedData();
}

#endif
