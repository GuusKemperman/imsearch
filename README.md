
# ImSearch

ImSearch is an immediate mode extension for [Dear ImGui](https://github.com/ocornut/imgui), allowing you to create searchbars for ImGui's buttons, trees, selectable, and every other widget you can imagine.

Just like ImGui, ImSearch's API does not burden the end user with state management, use STL containers or include any C++ headers, and has no public dependencies except for ImGui itself.

## Examples

<table>
  <tr>
    <th><img src="https://github.com/user-attachments/assets/3b2a9d01-7791-42cf-8df6-40eb2e26d0df"><p>Allows your user to search through </p> </th>
    <th><img src="https://github.com/user-attachments/assets/4680dc71-ffc5-42cd-aef5-bbacaf120541"><p>Supports hierarchies; ideal for content browsers, file explorers, or scene outliners.</p> </th>
  </tr>
  <tr>
    <th><img src="https://github.com/user-attachments/assets/c35471fe-8572-4238-a238-48d9a840ebb2"></th>
    <th><img src="https://github.com/user-attachments/assets/041394e7-e145-4df8-84e8-7a8ecb2e8371"></th>
  </tr>
</table>

## Usage

You'll find that the API still feels very familiar to ImGui, albeit *slightly* different from what you might expect from an ImGui extension; because this extension also takes control over the order to display items in, the library uses callbacks for submitting your widgets. First, start a new search context by calling `ImSearch::BeginSearch`. Next, submit as many items as you want with the Push/PopSearchable functions, to which you provide callbacks to display your ImGui widget (e.g. `Selectable`, `Button`, `TreeNode`, etc). Finally, wrap things up with a call to `ImSearch::EndSearch()`. That's it!

```cpp
static const char* selectedString = nouns[0];
if (ImGui::BeginCombo("Nouns", selectedString))
{
    if (ImSearch::BeginSearch())
    {
        ImSearch::SearchBar();
        for (const char* noun : nouns)
        {
            if (ImSearch::PushSearchable(noun,
                [&](const char* name)
                {
                    const bool isSelected = name == selectedString;
                    if (ImGui::Selectable(name, isSelected))
                    {
                        selectedString = name;
                    }
                    return true;
                }))
            {
                ImSearch::PopSearchable();
            }
        }

        ImSearch::EndSearch();
    }
    ImGui::EndCombo();
}
```

## Demos

More examples of ImSearch's features and usages can be found in `imsearch_demo.cpp`. Add this file to your sources and call `ImSearch::ShowDemoWindow()` somewhere in your update loop. You are encouraged to use this file as a reference whenever you need it. The demo is always updated to show new features as they are added, so check back with each release!

## Integration

0) Set up an [ImGui](https://github.com/ocornut/imgui) environment if you don't already have one.
1) Add `imsearch.h`, `imsearch_internal.h`, `imsearch.cpp`, and optionally `imsearch_demo.cpp` to your sources.
2) Create and destroy an `ImSearchContext` wherever you do so for your `ImGuiContext`:

```cpp
ImGui::CreateContext();
ImSearch::CreateContext();
...
ImSearch::DestroyContext();
ImGui::DestroyContext();
```

You should be good to go!
