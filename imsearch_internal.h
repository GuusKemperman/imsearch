#pragma once
#ifndef IMGUI_DISABLE

#include <string>
#include <cstddef>

namespace ImSearch
{
    struct StringMatcher
    {
        StringMatcher(const char* userQuery);

        float operator()(const char* textBody) const;

    private:
        std::string mProcessedUserQuery{};
    };
}


#endif // #ifndef IMGUI_DISABLE
