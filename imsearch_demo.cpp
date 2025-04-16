#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imgui.h"

void ImSearch::ShowDemoWindow(bool* p_open)
{
	if (!ImGui::Begin("ImSearch Demo", p_open))
	{
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("TODO: Create ImSearch!");
	ImGui::End();
}

#endif // #ifndef IMGUI_DISABLE
