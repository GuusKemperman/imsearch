// Sample app built with Dear ImGui and ImSearch
// This app uses imsearch and imgui, but does not output to any backend! It only serves as a proof that an app can be built, linked, and run.

#include "imgui.h"
#include "imsearch.h"
#include <cstdio>

int main(int, char**)
{    
    puts("sample_imsearch: start\n");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImSearch::CreateContext();

    // Additional imgui initialization needed when no backend is present
    ImGui::GetIO().DisplaySize = ImVec2(400.f, 400.f);
    ImGui::GetIO().Fonts->Build();

    // Render 500 frames
    for(int counter = 0; counter < 500; ++counter)
    {
        ImGui::NewFrame();
        ImSearch::ShowDemoWindow();
        ImGui::Render();
    }

    ImSearch::DestroyContext();
    ImGui::DestroyContext();
    puts("sample_imsearch: end\n");
    return 0;
}
