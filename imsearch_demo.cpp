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

	if (ImGui::TreeNode("Simple"))
	{
		ImSearch::BeginSearch();

		const char* arr[6]{ "John", "Susan", "Johnathan", "Bob", "Mandy", "Mary" };

		for (int i = 0; i < 6; i++)
		{
			ImSearch::PushSearchable(arr[i], 
				[](const char* str) 
				{ 
					ImGui::TextUnformatted(str);
					return true;
				});
			ImSearch::PopSearchable();
		}

		ImSearch::EndSearch();
		ImGui::TreePop();
	}

	ImGui::End();
}

#endif // #ifndef IMGUI_DISABLE
