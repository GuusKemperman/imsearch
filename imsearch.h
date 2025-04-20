#pragma once
#include <functional>

#ifndef IMGUI_DISABLE

namespace ImSearch
{
	/**
	 * \brief Adds a search bar. Always requires End() to be called.
	 */
	void BeginSearch();

	/**
	 * \brief Finishes up by starting to call the submitted display functions, in the order of relevancy to the user.
	 */
	void EndSearch();

	bool PushSearchable(const char* name, std::function<bool(const char*)> displayStart);

	void PopSearchable(std::function<void()> displayEnd = {});

	// Shows the ImSearch demo window (add imsearch_demo.cpp to your sources!)
	void ShowDemoWindow(bool* p_open = nullptr);

	void TextUnformatted(const char* text);

	bool TreeNode(const char* text);
	void TreePop();
}

#endif // #ifndef IMGUI_DISABLE
