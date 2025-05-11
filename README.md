
# ImSearch

ImSearch is an immediate mode extension for [Dear ImGui](https://github.com/ocornut/imgui), allowing you to create searchbars for ImGui's buttons, trees, selectable, and every other widget you can imagine.

Just like ImGui, ImSearch's API does not burden the end user with state management, use STL containers or include any C++ headers, and has no public dependencies except for ImGui itself. This extension is compatible with C++11 and above.

## Examples

<table>
  <tr>
    <th><img src="https://github.com/user-attachments/assets/f76ca584-ff01-47e8-855b-44ef7c7c3b73"><p>Combos, previews and autocomplete</p> </th>
    <th><img src="https://github.com/user-attachments/assets/967e6790-fc41-43cc-bd4d-f88d09f5a9f7"><p>Hierarchies</p> </th>
  </tr>
  <tr>
    <th><img src="https://github.com/user-attachments/assets/2748ebeb-37a5-4c10-8fd4-86b34e0f129a"><p>Collapsing headers and nested search bars</p></th>
    <th><img src="https://github.com/user-attachments/assets/99c2c3b3-859a-4a43-8a25-724a1dfbc912"><p>Custom search bars</p> </th>
  </tr>
</table>

<table>
  <tr>
    <th colspan = "3">
      <h3>Real world examples from <a href="https://github.com/GuusKemperman/CoralEngine">Coral Engine</a></h3>
    </th>
  <tr>
  <tr>
    <th><img src="https://github.com/user-attachments/assets/4659ab77-0265-4acf-8979-673e276e9fef"><p>World outliner, details panel, and asset selection</p> </th>
    <th><img src="https://github.com/user-attachments/assets/cc800604-d50c-4429-9f11-f8db91d12cfc"><p>Content browser</p> </th>
    <th><img src="https://github.com/user-attachments/assets/9c30763d-5c10-4275-8b57-7f2ac7a1a891"><p>Finding functions and types for easier scripting</p> </th>
  </tr>
</table>

## Usage

You'll find that the API still feels very familiar to ImGui, albeit *slightly* different from what you might expect from an ImGui extension; because this extension also takes control over the order to display items in, the library uses callbacks for submitting your widgets. First, start a new search context by calling `ImSearch::BeginSearch`. Next, submit as many items as you want with the Push/PopSearchable functions, to which you provide callbacks to display your ImGui widget (e.g. `Selectable`, `Button`, `TreeNode`, etc). Finally, wrap things up with a call to `ImSearch::EndSearch()`. That's it!

<table>

```cpp
static const char* selectedString = sImguiExtensions[0];

if (ImGui::BeginCombo("##Extensions", selectedString))
{
    if (ImSearch::BeginSearch())
    {
        ImSearch::SearchBar();

        for (const char* extension : sImguiExtensions)
        {
            ImSearch::SearchableItem(extension,
                [&](const char* name)
                {
                    const bool isSelected = name == selectedString;
                    if (ImGui::Selectable(name, isSelected))
                    {
                        selectedString = name;
                    }
                });
        }
        ImSearch::EndSearch();
    }
    ImGui::EndCombo();
}
```

![](https://github.com/user-attachments/assets/66b73ee6-3134-472e-889f-f8dcb81f9757)

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
