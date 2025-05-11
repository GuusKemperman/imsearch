#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imsearch_internal.h"
#include "imgui.h"

#include <chrono>
#include <array>
#include <string>
#include <functional>

namespace
{
	std::array<const char*, 100> nouns{ "people","history","way","art","world","information","map","two","family","government","health","system","computer","meat","year","thanks","music","person","reading","method","data","food","understanding","theory","law","bird","literature","problem","software","control","knowledge","power","ability","economics","love","internet","television","science","library","nature","fact","product","idea","temperature","investment","area","society","activity","story","industry","media","thing","oven","community","definition","safety","quality","development","language","management","player","variety","video","week","security","country","exam","movie","organization","equipment","physics","analysis","policy","series","thought","basis","boyfriend","direction","strategy","technology","army","camera","freedom","paper","environment","child","instance","month","truth","marketing","university","writing","article","department","difference","goal","news","audience","fishing","growth" };
	std::array<const char*, 100> adjectives{ "different","used","important","every","large","available","popular","able","basic","known","various","difficult","several","united","historical","hot","useful","mental","scared","additional","emotional","old","political","similar","healthy","financial","medical","traditional","federal","entire","strong","actual","significant","successful","electrical","expensive","pregnant","intelligent","interesting","poor","happy","responsible","cute","helpful","recent","willing","nice","wonderful","impossible","serious","huge","rare","technical","typical","competitive","critical","electronic","immediate","aware","educational","environmental","global","legal","relevant","accurate","capable","dangerous","dramatic","efficient","powerful","foreign","hungry","practical","psychological","severe","suitable","numerous","sufficient","unusual","consistent","cultural","existing","famous","pure","afraid","obvious","careful","latter","unhappy","acceptable","aggressive","boring","distinct","eastern","logical","reasonable","strict","administrative","automatic","civil" };
    std::array<const char*, 117> sImguiExtensions = { "imgui_test_engine","ImGuiColorTextEdit","Zep","Scintilla integration","imgui-node-editor","ImNodes","imnodes","ImNodeFlow","imgui_memory_editor","imgui_hex_editor","ImPlot","ImPlot3D","imgui-plot","SimpleImGuiFlameGraph","imgui-flame-graph","Plot Var Helper","Cubic Bezier / Curve Editor","ImSequencer","ImGradient","ImCurveEdit","Gradient Color Generator","im-neo-sequencer","HImGuiAnimation","ImFileDialog","imfile","ImGuiFD","L2DFileDialog","aiekick/ImGuiFileDialog","OverShifted's Directory tree view","AirGuanZ's imgui-filebrowser","gallickgunner's ImGui-Addons","Flix01's ImGui-Addons","imgui_markdown","imgui_md","UntitledImGuiTextUtils","url/hyperlinks","DearImGui-with-IMM32","UntitledIBusHandwriting","imgui-rs-knobs","imgui-knobs","imspinner","Spinner/Loading progress indicators","Toggle Button (under Toggles)","imgui_toggle","Splitters","Stack Layout (pr/branch)","ImRAD","ImStudio","ImGuiBuilder","ImGuiDesigner","Fellow ImGui","HImGuiEditor","Dear-Design-Manager","Thread","imgui-spectrum","Software Renderer for Dear ImGui","ImSoft","Fast Software Rasterizer","Backend for Xlux","netImGui","UnrealNetImgui","imgui-ws","ImGui_WS","RemoteImGui","AndroidAppViewer","ImTui","tear imgui","midi2osc","devmidi","shric/midi","Desktop+","ImGuiVR","BIMXplorer","mpFluid CAVE Front End","imgInspect","ImGuiTexInspect","ImGuiDatePicker","BrotBoxEngine's ImGuiExtensions.cpp","Flix01's ImGui-Addons","ImGuizmo","imGuiZMO.quat","ImGui-2D-HArrow","ImOGuizmo","ImGui::Auto()","ImQuick","imgui-inspect","imgui_stdlib","TextFmt()","imgui_scoped","imgui_sugar","imguiwrap","Explicit context pointer PR/patch","Multi-Context Compositor","Cog","PropertyWatcher","UnrealImGuiTools","UnrealNetImgui","SrgImGui","ImSearch","ImGuiTextSelect","ImZoomSlider","Slider 2D and Slider 3D","imgui-notify","ImHotKey","IP Entry Box","Pie Menu","nnview","ImGui Command Palette","imlottie","ImCoolBar","InAppGpuProfiler","Flix01's ImGui-Addons","@leiradel's snippets","@nem0's snippets","@aoterodelaroza's snippets","MetricsGui","nakedeyes' UnrealImGuiTools" };
	
    size_t Rand(size_t& seed);
	const char* GetRandomString(size_t& seed, std::string& str);

    void HelpMarker(const char* desc);

    bool ImSearchDemo_TreeNode(const char* name);
    void ImSearchDemo_TreeLeaf(const char* name);
    void ImSearchDemo_TreePop();

    bool ImSearchDemo_CollapsingHeader(const char* name);
}

#if defined(_MSVC_LANG)  // MS compiler has different __cplusplus value.
#   if _MSVC_LANG >= 201402L
		#define HAS_CPP14
#   endif
#else  // All other compilers.
#   if __cplusplus >= 201402L
		#define HAS_CPP14
#   endif
#endif

void ImSearch::ShowDemoWindow(bool* p_open)
{
	if (!ImGui::Begin("ImSearch Demo", p_open))
	{
		ImGui::End();
		return;
	}

	size_t seed = static_cast<size_t>(0xbadC0ffee);
	// Reuse the same string
	// when generating random strings,
	// to reduce heap allocations
	std::string randStr{};

	if (ImGui::TreeNode("Basic"))
	{
		if (ImSearch::BeginSearch())
		{
            ImSearch::SearchBar();

            ImSearch::SearchableItem("Hey there!",
                [](const char* name)
                {
                    ImGui::Selectable(name);
                    return true;
                });

            ImSearch::SearchableItem("Howdy partner!",
                [](const char* name)
                {
                    ImGui::Button(name);
                    ImGui::SetItemTooltip("Click me!");
                    return true;
                });

            ImSearch::EndSearch();
		}

		ImGui::TreePop();
	}

    if (ImGui::TreeNode("Combo"))
    {
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

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Custom Search bar"))
    {
        if (ImSearch::BeginSearch())
        {
        	static char query[2048]{};

            ImGui::SetNextItemWidth(-FLT_MIN);

            const float spaceWidth = ImGui::CalcTextSize(" ").x;
            const float searchbarWidth = ImGui::GetContentRegionAvail().x;
            const int totalNumCharacters = static_cast<int>(searchbarWidth / spaceWidth);

            const int timeAsInt = static_cast<int>(ImGui::GetTime() * 10.0);

            constexpr int length = 31;
            constexpr char hint[length + 1] = "I'm a custom search bar!       ";
            std::string hintWithSpacing{};

            for (int i = 0; i < totalNumCharacters; i++)
            {
                int index = (i + timeAsInt) % length;
                hintWithSpacing.push_back(hint[index]);
            }

            if (ImGui::InputTextWithHint("##Searchbar", hintWithSpacing.c_str(), query, sizeof(query)))
            {
                ImSearch::SetUserQuery(query);
            }

            for (int i = 0; i < 3; i++)
            {
                ImSearch::SearchableItem(GetRandomString(seed, randStr),
                    [](const char* str)
                    {
                        ImGui::Selectable(str);
                        return true;
                    });
            }

            ImSearch::EndSearch();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("How do callbacks work?"))
    {
        HelpMarker("This displayed section probably won't make a lot of sense if you're not also looking at the code behind it.");

        if (ImSearch::BeginSearch())
        {
            ImSearch::SearchBar();

            if (ImSearchDemo_CollapsingHeader("std::functions"))
            {
                std::function<bool(const char*)> myDisplayStart =
                    [](const char* str) -> bool
                    {
                        return ImGui::TreeNode(str);
                    };

                std::function<void()> myDisplayEnd =
                    []()
                    {
                        return ImGui::TreePop();
                    };

                if (ImSearch::PushSearchable("std::function!", myDisplayStart))
                {
                    ImSearch::PopSearchable(myDisplayEnd);
                }

                ImSearch::PopSearchable();
            }



#ifdef HAS_CPP14 // C++11 didnt support lambda captures by value. 
            if (ImSearchDemo_CollapsingHeader("Lambdas and captures"))
        	{
                const std::string tooltip = GetRandomString(seed, randStr);
                ImSearch::SearchableItem(GetRandomString(seed, randStr),
                    // You can capture anything in the lambda you might need.
                    // The easiest way, works with any C++ lambda.
                    [=](const char* str)
                    {
                        ImGui::TextUnformatted(str);

                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::TextUnformatted(tooltip.c_str());
                            ImGui::EndTooltip();
                        }
                    });

                ImSearch::PopSearchable();
            }
#endif

            if (ImSearchDemo_CollapsingHeader("Free functions"))
            {
            	// C++ may sometimes require you to cast, to avoid ambiguity between different overloads.
                if (ImSearch::PushSearchable("Tree", static_cast<bool(*)(const char* label)>(&ImGui::TreeNode)))
                {
                    // Sometimes you can even use functions directly from ImGui!
                    ImSearch::PopSearchable(&ImGui::TreePop);
                }

                ImSearch::PopSearchable();
            }

            if (ImSearchDemo_CollapsingHeader("Common pitfall: dangling references"))
			{
				{
                    int hiIWentOutOfScope{};

                    ImSearch::SearchableItem("Undefined behaviour, variable out of scope!",
                        [&hiIWentOutOfScope](const char* name) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                        {                                                                                       /*^*/
                            ImGui::TextUnformatted(name);                                                       /*^*/
																												/*^*/
                            // This would be invalid!                                                           /*^*/
                            // ImGui::InputInt("DontDoThis", &hiIWentOutOfScope);                               /*^*/
                                                                                                                /*^*/
                        	(void)(hiIWentOutOfScope); // (just to silence warnings of it being unused			/*^*/
                        });                                                                                     /*^*/
				}                                                                                               /*^*/
                                                                                                                /*^*/
                ImSearch::PopSearchable();                                                                      /*^*/
                                                                                                                /*^*/
            	// Your callbacks can be invoked at any point between your call to PushSearchable and the next  /*^*/
                // ImSearch::Submit or ImSearch::EndSearch is reached. Make sure your callbacks remain valid,   /*^*/
                // with nothing dangling.                                                                       /*^*/
				ImSearch::Submit(); // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>^
			}

            ImSearch::EndSearch();
        }

        ImGui::TreePop();
    }

	if (ImGui::TreeNode("Many"))
	{
		if (ImSearch::BeginSearch())
		{
            ImGui::TextWrapped("SearchBar's can be placed anywhere between BeginSearch and EndSearch; even outside the child window");
            ImSearch::SearchBar();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

            if (ImGui::BeginChild("Submissions", {}, ImGuiChildFlags_Borders))
            {
                for (int i = 0; i < 1000; i++)
                {
                    ImSearch::SearchableItem(GetRandomString(seed, randStr),
                        [](const char* str)
                        {
                            ImGui::TextUnformatted(str);
                        });
                }

                // Call Submit explicitly; all the callbacks
                // will be invoked through submit. If we
                // had waited for EndSearch to do this for us,
                // the callbacks would've been invoked after
                // ImGui::EndChild, leaving our searchables
                // to be displayed outside of the child window.
                ImSearch::Submit();
            } ImGui::EndChild();

            ImGui::PopStyleColor();
            ImSearch::EndSearch();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Tree Nodes"))
	{
        if (ImSearch::BeginSearch())
        {
            ImSearch::SearchBar();

            if (ImSearchDemo_TreeNode("Professions"))
            {
                if (ImSearchDemo_TreeNode("Farmers"))
                {
                    if (ImSearchDemo_TreeNode("Tools"))
                    {
                        ImSearchDemo_TreeLeaf("Hoe");
                        ImSearchDemo_TreeLeaf("Sickle");
                        ImSearchDemo_TreeLeaf("Plow");
                        ImSearchDemo_TreeLeaf("Wheelbarrow");
                        ImSearchDemo_TreeLeaf("Rake");
                        ImSearchDemo_TreeLeaf("Pitchfork");
                        ImSearchDemo_TreeLeaf("Scythe");
                        ImSearchDemo_TreeLeaf("Hand Trowel");
                        ImSearchDemo_TreeLeaf("Pruning Shears");
                        ImSearchDemo_TreeLeaf("Seed Drill");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Crops"))
                    {
                        ImSearchDemo_TreeLeaf("Wheat");
                        ImSearchDemo_TreeLeaf("Corn");
                        ImSearchDemo_TreeLeaf("Rice");
                        ImSearchDemo_TreeLeaf("Soybeans");
                        ImSearchDemo_TreeLeaf("Barley");
                        ImSearchDemo_TreeLeaf("Oats");
                        ImSearchDemo_TreeLeaf("Cotton");
                        ImSearchDemo_TreeLeaf("Sugarcane");
                        ImSearchDemo_TreeLeaf("Potatoes");
                        ImSearchDemo_TreeLeaf("Tomatoes");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Livestock"))
                    {
                        ImSearchDemo_TreeLeaf("Cattle");
                        ImSearchDemo_TreeLeaf("Sheep");
                        ImSearchDemo_TreeLeaf("Goats");
                        ImSearchDemo_TreeLeaf("Pigs");
                        ImSearchDemo_TreeLeaf("Chickens");
                        ImSearchDemo_TreeLeaf("Ducks");
                        ImSearchDemo_TreeLeaf("Horses");
                        ImSearchDemo_TreeLeaf("Bees");
                        ImSearchDemo_TreeLeaf("Turkeys");
                        ImSearchDemo_TreeLeaf("Llamas");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                if (ImSearchDemo_TreeNode("Blacksmiths"))
                {
                    if (ImSearchDemo_TreeNode("Tools"))
                    {
                        ImSearchDemo_TreeLeaf("Hammer");
                        ImSearchDemo_TreeLeaf("Anvil");
                        ImSearchDemo_TreeLeaf("Tongs");
                        ImSearchDemo_TreeLeaf("Forge");
                        ImSearchDemo_TreeLeaf("Quenching Tank");
                        ImSearchDemo_TreeLeaf("Files");
                        ImSearchDemo_TreeLeaf("Chisels");
                        ImSearchDemo_TreeLeaf("Punches");
                        ImSearchDemo_TreeLeaf("Swage Block");
                        ImSearchDemo_TreeLeaf("Bellows");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Materials"))
                    {
                        ImSearchDemo_TreeLeaf("Iron Ore");
                        ImSearchDemo_TreeLeaf("Coal");
                        ImSearchDemo_TreeLeaf("Charcoal");
                        ImSearchDemo_TreeLeaf("Steel Ingots");
                        ImSearchDemo_TreeLeaf("Copper");
                        ImSearchDemo_TreeLeaf("Bronze");
                        ImSearchDemo_TreeLeaf("Nickel");
                        ImSearchDemo_TreeLeaf("Cobalt");
                        ImSearchDemo_TreeLeaf("Manganese");
                        ImSearchDemo_TreeLeaf("Flux");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Products"))
                    {
                        ImSearchDemo_TreeLeaf("Horseshoes");
                        ImSearchDemo_TreeLeaf("Nails");
                        ImSearchDemo_TreeLeaf("Swords");
                        ImSearchDemo_TreeLeaf("Axes");
                        ImSearchDemo_TreeLeaf("Armor Plates");
                        ImSearchDemo_TreeLeaf("Tools");
                        ImSearchDemo_TreeLeaf("Chains");
                        ImSearchDemo_TreeLeaf("Iron Gates");
                        ImSearchDemo_TreeLeaf("Rail Tracks");
                        ImSearchDemo_TreeLeaf("Decorative Grills");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                if (ImSearchDemo_TreeNode("Fishermen"))
                {
                    if (ImSearchDemo_TreeNode("Equipment"))
                    {
                        ImSearchDemo_TreeLeaf("Fishing Rod");
                        ImSearchDemo_TreeLeaf("Net");
                        ImSearchDemo_TreeLeaf("Tackle Box");
                        ImSearchDemo_TreeLeaf("Hooks");
                        ImSearchDemo_TreeLeaf("Lures");
                        ImSearchDemo_TreeLeaf("Bobbers");
                        ImSearchDemo_TreeLeaf("Sinkers");
                        ImSearchDemo_TreeLeaf("Gaff");
                        ImSearchDemo_TreeLeaf("Gill Net");
                        ImSearchDemo_TreeLeaf("Crab Pot");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Catch"))
                    {
                        ImSearchDemo_TreeLeaf("Salmon");
                        ImSearchDemo_TreeLeaf("Tuna");
                        ImSearchDemo_TreeLeaf("Trout");
                        ImSearchDemo_TreeLeaf("Cod");
                        ImSearchDemo_TreeLeaf("Haddock");
                        ImSearchDemo_TreeLeaf("Shrimp");
                        ImSearchDemo_TreeLeaf("Crab");
                        ImSearchDemo_TreeLeaf("Lobster");
                        ImSearchDemo_TreeLeaf("Sardines");
                        ImSearchDemo_TreeLeaf("Mussels");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Boats"))
                    {
                        ImSearchDemo_TreeLeaf("Rowboat");
                        ImSearchDemo_TreeLeaf("Sailboat");
                        ImSearchDemo_TreeLeaf("Trawler");
                        ImSearchDemo_TreeLeaf("Catamaran");
                        ImSearchDemo_TreeLeaf("Kayak");
                        ImSearchDemo_TreeLeaf("Dinghy");
                        ImSearchDemo_TreeLeaf("Canoe");
                        ImSearchDemo_TreeLeaf("Fishing Trawler");
                        ImSearchDemo_TreeLeaf("Longliner");
                        ImSearchDemo_TreeLeaf("Gillnetter");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                ImSearchDemo_TreePop();
            }

            if (ImSearchDemo_TreeNode("Technologies"))
            {
                if (ImSearchDemo_TreeNode("Computers"))
                {
                    if (ImSearchDemo_TreeNode("Hardware"))
                    {
                        ImSearchDemo_TreeLeaf("CPU");
                        ImSearchDemo_TreeLeaf("GPU");
                        ImSearchDemo_TreeLeaf("RAM");
                        ImSearchDemo_TreeLeaf("Motherboard");
                        ImSearchDemo_TreeLeaf("SSD");
                        ImSearchDemo_TreeLeaf("HDD");
                        ImSearchDemo_TreeLeaf("Power Supply");
                        ImSearchDemo_TreeLeaf("Cooler");
                        ImSearchDemo_TreeLeaf("Case");
                        ImSearchDemo_TreeLeaf("Network Card");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Software"))
                    {
                        ImSearchDemo_TreeLeaf("Operating System");
                        ImSearchDemo_TreeLeaf("Web Browser");
                        ImSearchDemo_TreeLeaf("Office Suite");
                        ImSearchDemo_TreeLeaf("IDE");
                        ImSearchDemo_TreeLeaf("Antivirus");
                        ImSearchDemo_TreeLeaf("Drivers");
                        ImSearchDemo_TreeLeaf("Database");
                        ImSearchDemo_TreeLeaf("Virtual Machine");
                        ImSearchDemo_TreeLeaf("Compiler");
                        ImSearchDemo_TreeLeaf("Text Editor");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Networking"))
                    {
                        ImSearchDemo_TreeLeaf("Router");
                        ImSearchDemo_TreeLeaf("Switch");
                        ImSearchDemo_TreeLeaf("Firewall");
                        ImSearchDemo_TreeLeaf("Modem");
                        ImSearchDemo_TreeLeaf("Access Point");
                        ImSearchDemo_TreeLeaf("Ethernet Cable");
                        ImSearchDemo_TreeLeaf("Fiber Optic Cable");
                        ImSearchDemo_TreeLeaf("VPN");
                        ImSearchDemo_TreeLeaf("DNS");
                        ImSearchDemo_TreeLeaf("DHCP");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                if (ImSearchDemo_TreeNode("Vehicles"))
                {
                    if (ImSearchDemo_TreeNode("Land"))
                    {
                        ImSearchDemo_TreeLeaf("Car");
                        ImSearchDemo_TreeLeaf("Truck");
                        ImSearchDemo_TreeLeaf("Motorcycle");
                        ImSearchDemo_TreeLeaf("Bicycle");
                        ImSearchDemo_TreeLeaf("Bus");
                        ImSearchDemo_TreeLeaf("Train");
                        ImSearchDemo_TreeLeaf("Tram");
                        ImSearchDemo_TreeLeaf("Tank");
                        ImSearchDemo_TreeLeaf("ATV");
                        ImSearchDemo_TreeLeaf("Segway");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Air"))
                    {
                        ImSearchDemo_TreeLeaf("Airplane");
                        ImSearchDemo_TreeLeaf("Helicopter");
                        ImSearchDemo_TreeLeaf("Drone");
                        ImSearchDemo_TreeLeaf("Glider");
                        ImSearchDemo_TreeLeaf("Hot Air Balloon");
                        ImSearchDemo_TreeLeaf("Jet");
                        ImSearchDemo_TreeLeaf("Blimp");
                        ImSearchDemo_TreeLeaf("Autogyro");
                        ImSearchDemo_TreeLeaf("Seaplane");
                        ImSearchDemo_TreeLeaf("Hang Glider");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Sea"))
                    {
                        ImSearchDemo_TreeLeaf("Ship");
                        ImSearchDemo_TreeLeaf("Boat");
                        ImSearchDemo_TreeLeaf("Submarine");
                        ImSearchDemo_TreeLeaf("Yacht");
                        ImSearchDemo_TreeLeaf("Canoe");
                        ImSearchDemo_TreeLeaf("Ferry");
                        ImSearchDemo_TreeLeaf("Sailboat");
                        ImSearchDemo_TreeLeaf("Tugboat");
                        ImSearchDemo_TreeLeaf("Catamaran");
                        ImSearchDemo_TreeLeaf("Dinghy");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                ImSearchDemo_TreePop();
            }

            if (ImSearchDemo_TreeNode("Nature"))
            {
                if (ImSearchDemo_TreeNode("Animals"))
                {
                    if (ImSearchDemo_TreeNode("Mammals"))
                    {
                        ImSearchDemo_TreeLeaf("Lion");
                        ImSearchDemo_TreeLeaf("Tiger");
                        ImSearchDemo_TreeLeaf("Elephant");
                        ImSearchDemo_TreeLeaf("Whale");
                        ImSearchDemo_TreeLeaf("Dolphin");
                        ImSearchDemo_TreeLeaf("Bat");
                        ImSearchDemo_TreeLeaf("Kangaroo");
                        ImSearchDemo_TreeLeaf("Human");
                        ImSearchDemo_TreeLeaf("Bear");
                        ImSearchDemo_TreeLeaf("Wolf");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Birds"))
                    {
                        ImSearchDemo_TreeLeaf("Eagle");
                        ImSearchDemo_TreeLeaf("Sparrow");
                        ImSearchDemo_TreeLeaf("Penguin");
                        ImSearchDemo_TreeLeaf("Owl");
                        ImSearchDemo_TreeLeaf("Parrot");
                        ImSearchDemo_TreeLeaf("Flamingo");
                        ImSearchDemo_TreeLeaf("Duck");
                        ImSearchDemo_TreeLeaf("Goose");
                        ImSearchDemo_TreeLeaf("Hawk");
                        ImSearchDemo_TreeLeaf("Crow");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Reptiles"))
                    {
                        ImSearchDemo_TreeLeaf("Crocodile");
                        ImSearchDemo_TreeLeaf("Snake");
                        ImSearchDemo_TreeLeaf("Lizard");
                        ImSearchDemo_TreeLeaf("Turtle");
                        ImSearchDemo_TreeLeaf("Chameleon");
                        ImSearchDemo_TreeLeaf("Gecko");
                        ImSearchDemo_TreeLeaf("Alligator");
                        ImSearchDemo_TreeLeaf("Komodo Dragon");
                        ImSearchDemo_TreeLeaf("Iguana");
                        ImSearchDemo_TreeLeaf("Rattlesnake");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }
                if (ImSearchDemo_TreeNode("Plants"))
                {
                    if (ImSearchDemo_TreeNode("Trees"))
                    {
                        ImSearchDemo_TreeLeaf("Oak");
                        ImSearchDemo_TreeLeaf("Pine");
                        ImSearchDemo_TreeLeaf("Maple");
                        ImSearchDemo_TreeLeaf("Birch");
                        ImSearchDemo_TreeLeaf("Cedar");
                        ImSearchDemo_TreeLeaf("Redwood");
                        ImSearchDemo_TreeLeaf("Palm");
                        ImSearchDemo_TreeLeaf("Willow");
                        ImSearchDemo_TreeLeaf("Spruce");
                        ImSearchDemo_TreeLeaf("Cypress");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Flowers"))
                    {
                        ImSearchDemo_TreeLeaf("Rose");
                        ImSearchDemo_TreeLeaf("Tulip");
                        ImSearchDemo_TreeLeaf("Sunflower");
                        ImSearchDemo_TreeLeaf("Daisy");
                        ImSearchDemo_TreeLeaf("Orchid");
                        ImSearchDemo_TreeLeaf("Lily");
                        ImSearchDemo_TreeLeaf("Marigold");
                        ImSearchDemo_TreeLeaf("Daffodil");
                        ImSearchDemo_TreeLeaf("Chrysanthemum");
                        ImSearchDemo_TreeLeaf("Iris");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Fungi"))
                    {
                        ImSearchDemo_TreeLeaf("Button Mushroom");
                        ImSearchDemo_TreeLeaf("Shiitake");
                        ImSearchDemo_TreeLeaf("Oyster Mushroom");
                        ImSearchDemo_TreeLeaf("Morel");
                        ImSearchDemo_TreeLeaf("Chanterelle");
                        ImSearchDemo_TreeLeaf("Truffle");
                        ImSearchDemo_TreeLeaf("Fly Agaric");
                        ImSearchDemo_TreeLeaf("Porcini");
                        ImSearchDemo_TreeLeaf("Puffball");
                        ImSearchDemo_TreeLeaf("Enoki");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                ImSearchDemo_TreePop();
            }
            if (ImSearchDemo_TreeNode("Culinary"))
            {
                if (ImSearchDemo_TreeNode("Ingredients"))
                {
                    if (ImSearchDemo_TreeNode("Spices"))
                    {
                        ImSearchDemo_TreeLeaf("Salt");
                        ImSearchDemo_TreeLeaf("Pepper");
                        ImSearchDemo_TreeLeaf("Paprika");
                        ImSearchDemo_TreeLeaf("Cumin");
                        ImSearchDemo_TreeLeaf("Turmeric");
                        ImSearchDemo_TreeLeaf("Oregano");
                        ImSearchDemo_TreeLeaf("Basil");
                        ImSearchDemo_TreeLeaf("Thyme");
                        ImSearchDemo_TreeLeaf("Cinnamon");
                        ImSearchDemo_TreeLeaf("Nutmeg");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Produce"))
                    {
                        ImSearchDemo_TreeLeaf("Carrot");
                        ImSearchDemo_TreeLeaf("Onion");
                        ImSearchDemo_TreeLeaf("Garlic");
                        ImSearchDemo_TreeLeaf("Pepper");
                        ImSearchDemo_TreeLeaf("Tomato");
                        ImSearchDemo_TreeLeaf("Lettuce");
                        ImSearchDemo_TreeLeaf("Spinach");
                        ImSearchDemo_TreeLeaf("Broccoli");
                        ImSearchDemo_TreeLeaf("Eggplant");
                        ImSearchDemo_TreeLeaf("Zucchini");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Proteins"))
                    {
                        ImSearchDemo_TreeLeaf("Chicken");
                        ImSearchDemo_TreeLeaf("Beef");
                        ImSearchDemo_TreeLeaf("Pork");
                        ImSearchDemo_TreeLeaf("Tofu");
                        ImSearchDemo_TreeLeaf("Lentils");
                        ImSearchDemo_TreeLeaf("Fish");
                        ImSearchDemo_TreeLeaf("Eggs");
                        ImSearchDemo_TreeLeaf("Beans");
                        ImSearchDemo_TreeLeaf("Lamb");
                        ImSearchDemo_TreeLeaf("Turkey");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }
                if (ImSearchDemo_TreeNode("Recipes"))
                {
                    if (ImSearchDemo_TreeNode("Soups"))
                    {
                        ImSearchDemo_TreeLeaf("Chicken Noodle Soup");
                        ImSearchDemo_TreeLeaf("Tomato Soup");
                        ImSearchDemo_TreeLeaf("Miso Soup");
                        ImSearchDemo_TreeLeaf("Minestrone");
                        ImSearchDemo_TreeLeaf("Clam Chowder");
                        ImSearchDemo_TreeLeaf("Pho");
                        ImSearchDemo_TreeLeaf("Ramen");
                        ImSearchDemo_TreeLeaf("Gazpacho");
                        ImSearchDemo_TreeLeaf("Pumpkin Soup");
                        ImSearchDemo_TreeLeaf("Lentil Soup");
                        ImSearchDemo_TreePop();
                    }
                    if (ImSearchDemo_TreeNode("Desserts"))
                    {
                        ImSearchDemo_TreeLeaf("Chocolate Cake");
                        ImSearchDemo_TreeLeaf("Apple Pie");
                        ImSearchDemo_TreeLeaf("Ice Cream");
                        ImSearchDemo_TreeLeaf("Brownies");
                        ImSearchDemo_TreeLeaf("Cheesecake");
                        ImSearchDemo_TreeLeaf("Pudding");
                        ImSearchDemo_TreeLeaf("Tiramisu");
                        ImSearchDemo_TreeLeaf("Crepes");
                        ImSearchDemo_TreeLeaf("Cupcakes");
                        ImSearchDemo_TreeLeaf("Macarons");
                        ImSearchDemo_TreePop();
                    }
                    ImSearchDemo_TreePop();
                }

                ImSearchDemo_TreePop();
            }

            ImSearch::EndSearch();
        }

		ImGui::TreePop();
	}

    if (ImGui::TreeNode("Collapsing headers"))
    {
        if (ImSearch::BeginSearch())
        {
            ImSearch::SearchBar();

            if (ImSearchDemo_CollapsingHeader("TransformComponent"))
            {
                ImSearch::SearchableItem("Position",
                    [](const char* name)
                    {
                        static float v[3]{};
                        ImGui::InputFloat3(name, v);
                        return true;
                    });

                ImSearch::SearchableItem("Scale",
                    [](const char* name)
                    {
                        static float v[3]{};
                        ImGui::InputFloat3(name, v);
                    });

                ImSearch::SearchableItem("Orientation",
                    [](const char* name)
                    {
                        static float v[3]{};
                        ImGui::InputFloat3(name, v);
                    });

                ImSearch::PopSearchable();
            }

            if (ImSearchDemo_CollapsingHeader("StaticMeshComponent"))
            {
                ImSearch::SearchableItem("Mesh",
                    [](const char* fieldName)
                    {
                        static const char* selectedString = nouns[0];
                        if (ImGui::BeginCombo(fieldName, selectedString))
                        {
                            if (ImSearch::BeginSearch())
                            {
                                ImSearch::SearchBar();
                                for (const char* noun : nouns)
                                {
                                    ImSearch::SearchableItem(noun,
                                        [&](const char* meshName)
                                        {
                                            const bool isSelected = meshName == selectedString;
                                            if (ImGui::Selectable(meshName, isSelected))
                                            {
                                                selectedString = meshName;
                                            }
                                            return true;
                                        });
                                }

                                ImSearch::EndSearch();
                            }
                            ImGui::EndCombo();
                        }
                    });

                ImSearch::PopSearchable();
            }

        	if (ImSearchDemo_CollapsingHeader("PhysicsBodyComponent"))
            {
                ImSearch::SearchableItem("Mass",
                    [](const char* name)
                    {
                        static float v{};
                        ImGui::InputFloat(name, &v);
                    });

                ImSearch::SearchableItem("Collision Enabled",
                    [](const char* name)
                    {
                        static bool b{};
                        ImGui::Checkbox(name, &b);
                    });

                ImSearch::PopSearchable();
            }

            ImSearch::EndSearch();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Artificially increasing priority for popular items"))
    {
        if (ImSearch::BeginSearch())
        {
            HelpMarker(
                "A bonus can be applied which, ONLY when the user is searching,\n"
				"influences which items are shown to the user first.\n\n"
				"Gus Goose is textually irrelevant, but because of his high\n"
				"bonus, he is the first result.");
            ImSearch::SearchBar();
            static bool doOnceDummy = []{ ImSearch::SetUserQuery("Duck"); return true; }();
            (void)doOnceDummy;

			static std::array<std::pair<const char*, float>, 15> namesAndBonuses
			{
			    std::pair<const char*, float>{ "Scrooge McDuck", 0.95f },             
			    std::pair<const char*, float>{ "Della Duck", 0.88f },                 
			    std::pair<const char*, float>{ "Huey Dewey & Louie Duck", 0.93f },    
			    std::pair<const char*, float>{ "Donald Duck", 1.0f },     
			    std::pair<const char*, float>{ "Grandma Duck", 0.82f },
			    std::pair<const char*, float>{ "Gus Goose", 5.0f },
			    std::pair<const char*, float>{ "Sir Quackly McDuck", 0.68f },         
			    std::pair<const char*, float>{ "Fethry Duck", 0.75f },                
			    std::pair<const char*, float>{ "Dugan Duck", 0.65f },                 
			    std::pair<const char*, float>{ "Sir Roast McDuck", 0.42f },            
			    std::pair<const char*, float>{ "Dudly D. Duck", 0.45f },              
			    std::pair<const char*, float>{ "Matilda McDuck", 0.58f },
			    std::pair<const char*, float>{ "Donna Duck", 0.55f },                 
			    std::pair<const char*, float>{ "Pothole McDuck", 0.52f },             
			    std::pair<const char*, float>{ "Daphne Duck-Gander", 0.60f },         
			};

            for (auto& nameAndBonus : namesAndBonuses)
            {
                if (ImSearch::PushSearchable(nameAndBonus.first,
                    [&](const char* name)
                    {
                        ImGui::SliderFloat(name, &nameAndBonus.second, -1.0f, 1.0f);
                        return false;
                    }))
                {
                    ImSearch::SetRelevancyBonus(nameAndBonus.second);
                    ImSearch::PopSearchable();
                }
            }

            ImSearch::EndSearch();
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Synonyms"))
    {
        if (ImSearch::BeginSearch())
        {
            HelpMarker("ImSearch supports synonyms, useful, if no one can agree on a single name. "
					   
						"For example, \"Class\" has the synonyms \"Type\" and \"Struct\"."
						"Try searching for \"Struct\"; you'll see \"Class\" rise up to the top!");
            ImSearch::SearchBar();

            auto selectableCallback = [](const char* name) { ImGui::Selectable(name); return false; };

            if (ImSearch::PushSearchable("Branch", selectableCallback))
            {
                ImSearch::AddSynonym("If");
                ImSearch::AddSynonym("Condition");

	            ImSearch::PopSearchable();
            }

            if (ImSearch::PushSearchable("Function", selectableCallback))
            {
                ImSearch::AddSynonym("Method");
                ImSearch::AddSynonym("Procedure");

                ImSearch::PopSearchable();
            }

        	if (ImSearch::PushSearchable("Variable", selectableCallback))
            {
                ImSearch::AddSynonym("Identifier");
                ImSearch::AddSynonym("Container");

                ImSearch::PopSearchable();
            }

            if (ImSearch::PushSearchable("Array", selectableCallback))
            {
                ImSearch::AddSynonym("List");
                ImSearch::AddSynonym("Collection");

                ImSearch::PopSearchable();
            }

            if (ImSearch::PushSearchable("Class", selectableCallback))
            {
                ImSearch::AddSynonym("Type");
                ImSearch::AddSynonym("Struct");

                ImSearch::PopSearchable();
            }

            ImSearch::EndSearch();
        }

        ImGui::TreePop();
    }

	ImGui::End();
}

namespace
{
	size_t Rand(size_t& seed)
	{
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return seed;
	}

	const char* GetRandomString(size_t& seed, std::string& str)
	{
		const char* adjective = adjectives[Rand(seed) % adjectives.size()];
		const char* noun = nouns[Rand(seed) % nouns.size()];

		str.clear();
		str.append(adjective);
		str.push_back(' ');
		str.append(noun);

		return str.c_str();
	}

    void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    bool ImSearchDemo_TreeNode(const char* name)
    {
        return ImSearch::PushSearchable(name, 
            [](const char* nodeName)
            {
                return ImGui::TreeNode(nodeName);
            });
    }

    void ImSearchDemo_TreeLeaf(const char* name)
    {
        ImSearch::SearchableItem(name,
            [](const char* leafName)
            {
                ImGui::Selectable(leafName);
            });
    }

    void ImSearchDemo_TreePop()
    {
        ImSearch::PopSearchable(
            []()
            {
                ImGui::TreePop();
            });
    }

    bool ImSearchDemo_CollapsingHeader(const char* name)
    {
        return ImSearch::PushSearchable(name, 
            [](const char* headerName)
            {
                return ImGui::CollapsingHeader(headerName);
            });
    }
}
#endif // #ifndef IMGUI_DISABLE
