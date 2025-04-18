#pragma once

#ifndef IMGUI_DISABLE

namespace ImSearch
{
    struct StringMatcher
    {
        StringMatcher(const char* userQuery);

        float operator()(const char* textBody) const;

    private:
        const char* mUserQuery{};
        size_t mUserLen{};
    };
}


#endif // #ifndef IMGUI_DISABLE
