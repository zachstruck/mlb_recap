#ifndef FEED_LOADER_HPP
#define FEED_LOADER_HPP

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

    MlbData getFeedData();
}

#endif
