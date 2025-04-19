#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imgui.h"

#include <array>
#include <string>

namespace
{
	std::array<const char*, 100> nouns{ "people","history","way","art","world","information","map","two","family","government","health","system","computer","meat","year","thanks","music","person","reading","method","data","food","understanding","theory","law","bird","literature","problem","software","control","knowledge","power","ability","economics","love","internet","television","science","library","nature","fact","product","idea","temperature","investment","area","society","activity","story","industry","media","thing","oven","community","definition","safety","quality","development","language","management","player","variety","video","week","security","country","exam","movie","organization","equipment","physics","analysis","policy","series","thought","basis","boyfriend","direction","strategy","technology","army","camera","freedom","paper","environment","child","instance","month","truth","marketing","university","writing","article","department","difference","goal","news","audience","fishing","growth" };
	std::array<const char*, 100> adjectives{ "different","used","important","every","large","available","popular","able","basic","known","various","difficult","several","united","historical","hot","useful","mental","scared","additional","emotional","old","political","similar","healthy","financial","medical","traditional","federal","entire","strong","actual","significant","successful","electrical","expensive","pregnant","intelligent","interesting","poor","happy","responsible","cute","helpful","recent","willing","nice","wonderful","impossible","serious","huge","rare","technical","typical","competitive","critical","electronic","immediate","aware","educational","environmental","global","legal","relevant","accurate","capable","dangerous","dramatic","efficient","powerful","foreign","hungry","practical","psychological","severe","suitable","numerous","sufficient","unusual","consistent","cultural","existing","famous","pure","afraid","obvious","careful","latter","unhappy","acceptable","aggressive","boring","distinct","eastern","logical","reasonable","strict","administrative","automatic","civil" };

	const char* GetRandomString(size_t& seed, std::string& str);
}

void ImSearch::ShowDemoWindow(bool* p_open)
{
	if (!ImGui::Begin("ImSearch Demo", p_open))
	{
		ImGui::End();
		return;
	}

	size_t seed = 0xbadC0ffee;

	// Reuse the same string
	// when generating random strings,
	// to reduce heap allocations
	std::string str{};

	if (ImGui::TreeNode("Simple"))
	{
		ImSearch::BeginSearch();

		for (int i = 0; i < 6; i++)
		{
			ImSearch::PushSearchable(GetRandomString(seed, str),
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

	if (ImGui::TreeNode("Many"))
	{
		ImSearch::BeginSearch();

		for (int i = 0; i < 10000; i++)
		{
			ImSearch::PushSearchable(GetRandomString(seed, str),
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


namespace
{
	const char* GetRandomString(size_t& seed, std::string& str)
	{
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;

		const char* adjective = adjectives[seed % adjectives.size()];

		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;

		const char* noun = nouns[seed % nouns.size()];

		str.clear();
		str.append(adjective);
		str.push_back(' ');
		str.append(noun);

		return str.c_str();
	}
}


#endif // #ifndef IMGUI_DISABLE
