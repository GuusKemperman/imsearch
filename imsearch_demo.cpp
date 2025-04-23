#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imgui.h"

#include <array>
#include <string>

namespace
{
	std::array<const char*, 100> nouns{ "people","history","way","art","world","information","map","two","family","government","health","system","computer","meat","year","thanks","music","person","reading","method","data","food","understanding","theory","law","bird","literature","problem","software","control","knowledge","power","ability","economics","love","internet","television","science","library","nature","fact","product","idea","temperature","investment","area","society","activity","story","industry","media","thing","oven","community","definition","safety","quality","development","language","management","player","variety","video","week","security","country","exam","movie","organization","equipment","physics","analysis","policy","series","thought","basis","boyfriend","direction","strategy","technology","army","camera","freedom","paper","environment","child","instance","month","truth","marketing","university","writing","article","department","difference","goal","news","audience","fishing","growth" };
	std::array<const char*, 100> adjectives{ "different","used","important","every","large","available","popular","able","basic","known","various","difficult","several","united","historical","hot","useful","mental","scared","additional","emotional","old","political","similar","healthy","financial","medical","traditional","federal","entire","strong","actual","significant","successful","electrical","expensive","pregnant","intelligent","interesting","poor","happy","responsible","cute","helpful","recent","willing","nice","wonderful","impossible","serious","huge","rare","technical","typical","competitive","critical","electronic","immediate","aware","educational","environmental","global","legal","relevant","accurate","capable","dangerous","dramatic","efficient","powerful","foreign","hungry","practical","psychological","severe","suitable","numerous","sufficient","unusual","consistent","cultural","existing","famous","pure","afraid","obvious","careful","latter","unhappy","acceptable","aggressive","boring","distinct","eastern","logical","reasonable","strict","administrative","automatic","civil" };

	size_t Rand(size_t& seed);
	const char* GetRandomString(size_t& seed, std::string& str);
}

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
	std::string str{};

	if (ImGui::TreeNode("Simple"))
	{
		ImSearch::BeginSearch();
        ImSearch::SearchBar();

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

    if (ImGui::TreeNode("Search bar"))
    {
        ImSearch::BeginSearch();

        ImGui::TextUnformatted("Custom search bar!");
        static char query[2048]{};

        if (ImGui::InputTextMultiline("This is a search bar!", query, sizeof(query)))
        {
            ImSearch::SetUserQuery(query);
        }
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



#if __cplusplus >= 201402L
    if (ImGui::TreeNode("Different functors"))
    {
        ImSearch::BeginSearch();
        ImSearch::SearchBar();

        for (int i = 0; i < 10; i++)
        {
            const std::string tooltip = GetRandomString(seed, str);
            ImSearch::PushSearchable(GetRandomString(seed, str),
                // You can capture anything in the lambda you might need.
                // Note that this requires C++14 or above
                [capturedTooltip = tooltip](const char* str)
                {
                    ImGui::TextUnformatted(str);

                    if (ImGui::BeginItemTooltip())
                    {
                        ImGui::TextUnformatted(capturedTooltip.c_str());
                        ImGui::EndTooltip();
                    }
                	return true;
                });
            ImSearch::PopSearchable();
        }

        ImSearch::EndSearch();
        ImGui::TreePop();
    }
#endif

	if (ImGui::TreeNode("Many"))
	{
		ImSearch::BeginSearch();
        ImGui::TextUnformatted("SearchBar's can be placed anywhere between BeginSearch and EndSearch; even outside the child window");
        ImSearch::SearchBar();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

        if (ImGui::BeginChild("Submissions", {}, ImGuiChildFlags_Borders))
        {
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
		ImGui::TreePop();
	}

    if (ImGui::TreeNode("Simple hierarchy"))
    {
        ImSearch::BeginSearch();
        ImSearch::SearchBar();

        if (ImSearch::TreeNode("I'm the root!"))
        {
            if (ImSearch::TreeNode("I'm the first kid, I got children of my own:"))
            {
                if (ImSearch::TreeNode("Hi! I'm a kid too. Here are my kids:"))
                {
					ImSearch::TextUnformatted("Amzy");
					ImSearch::TextUnformatted("Alfonso");
					ImSearch::TextUnformatted("Kay");
					ImSearch::TextUnformatted("Leo");
					ImSearch::TextUnformatted("Marcin");
					ImSearch::TextUnformatted("Nikola");
					ImSearch::TextUnformatted("Jelle");
					ImSearch::TextUnformatted("Orhan");
                    ImSearch::TextUnformatted("Stez");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }
            ImSearch::TextUnformatted("I'm the second kid, I have no children :(");

            ImSearch::TreePop();
        }

        ImSearch::EndSearch();
        ImGui::TreePop();
    }

	if (ImGui::TreeNode("Large hierarchy"))
	{
		ImSearch::BeginSearch();
        ImSearch::SearchBar();

        if (ImSearch::TreeNode("Professions"))
        {
            if (ImSearch::TreeNode("Farmers"))
            {
                if (ImSearch::TreeNode("Tools"))
                {
                    ImSearch::TextUnformatted("Hoe, for tilling soil and removing weeds.");
                    ImSearch::TextUnformatted("Sickle, for cutting crops like wheat or grass.");
                    ImSearch::TextUnformatted("Plow, for turning and loosening soil before planting.");
                    ImSearch::TextUnformatted("Wheelbarrow, for transporting soil, crops, and tools.");
                    ImSearch::TextUnformatted("Rake, for leveling soil or gathering leaves and hay.");
                    ImSearch::TextUnformatted("Pitchfork, for moving hay, compost, or manure.");
                    ImSearch::TextUnformatted("Scythe, a long-handled tool for mowing grass or reaping crops.");
                    ImSearch::TextUnformatted("Hand Trowel, for small digging jobs like planting.");
                    ImSearch::TextUnformatted("Pruning Shears, for trimming plants and branches.");
                    ImSearch::TextUnformatted("Seed Drill, for planting seeds at consistent depth and spacing.");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Crops"))
                {
                    ImSearch::TextUnformatted("Wheat");
                    ImSearch::TextUnformatted("Corn");
                    ImSearch::TextUnformatted("Rice");
                    ImSearch::TextUnformatted("Soybeans");
                    ImSearch::TextUnformatted("Barley");
                    ImSearch::TextUnformatted("Oats");
                    ImSearch::TextUnformatted("Cotton");
                    ImSearch::TextUnformatted("Sugarcane");
                    ImSearch::TextUnformatted("Potatoes");
                    ImSearch::TextUnformatted("Tomatoes");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Livestock"))
                {
                    ImSearch::TextUnformatted("Cattle");
                    ImSearch::TextUnformatted("Sheep");
                    ImSearch::TextUnformatted("Goats");
                    ImSearch::TextUnformatted("Pigs");
                    ImSearch::TextUnformatted("Chickens");
                    ImSearch::TextUnformatted("Ducks");
                    ImSearch::TextUnformatted("Horses");
                    ImSearch::TextUnformatted("Bees");
                    ImSearch::TextUnformatted("Turkeys");
                    ImSearch::TextUnformatted("Llamas");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            if (ImSearch::TreeNode("Blacksmiths"))
            {
                if (ImSearch::TreeNode("Tools"))
                {
                    ImSearch::TextUnformatted("Hammer");
                    ImSearch::TextUnformatted("Anvil");
                    ImSearch::TextUnformatted("Tongs");
                    ImSearch::TextUnformatted("Forge");
                    ImSearch::TextUnformatted("Quenching Tank");
                    ImSearch::TextUnformatted("Files");
                    ImSearch::TextUnformatted("Chisels");
                    ImSearch::TextUnformatted("Punches");
                    ImSearch::TextUnformatted("Swage Block");
                    ImSearch::TextUnformatted("Bellows");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Materials"))
                {
                    ImSearch::TextUnformatted("Iron Ore");
                    ImSearch::TextUnformatted("Coal");
                    ImSearch::TextUnformatted("Charcoal");
                    ImSearch::TextUnformatted("Steel Ingots");
                    ImSearch::TextUnformatted("Copper");
                    ImSearch::TextUnformatted("Bronze");
                    ImSearch::TextUnformatted("Nickel");
                    ImSearch::TextUnformatted("Cobalt");
                    ImSearch::TextUnformatted("Manganese");
                    ImSearch::TextUnformatted("Flux");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Products"))
                {
                    ImSearch::TextUnformatted("Horseshoes");
                    ImSearch::TextUnformatted("Nails");
                    ImSearch::TextUnformatted("Swords");
                    ImSearch::TextUnformatted("Axes");
                    ImSearch::TextUnformatted("Armor Plates");
                    ImSearch::TextUnformatted("Tools");
                    ImSearch::TextUnformatted("Chains");
                    ImSearch::TextUnformatted("Iron Gates");
                    ImSearch::TextUnformatted("Rail Tracks");
                    ImSearch::TextUnformatted("Decorative Grills");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            if (ImSearch::TreeNode("Fishermen"))
            {
                if (ImSearch::TreeNode("Equipment"))
                {
                    ImSearch::TextUnformatted("Fishing Rod");
                    ImSearch::TextUnformatted("Net");
                    ImSearch::TextUnformatted("Tackle Box");
                    ImSearch::TextUnformatted("Hooks");
                    ImSearch::TextUnformatted("Lures");
                    ImSearch::TextUnformatted("Bobbers");
                    ImSearch::TextUnformatted("Sinkers");
                    ImSearch::TextUnformatted("Gaff");
                    ImSearch::TextUnformatted("Gill Net");
                    ImSearch::TextUnformatted("Crab Pot");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Catch"))
                {
                    ImSearch::TextUnformatted("Salmon");
                    ImSearch::TextUnformatted("Tuna");
                    ImSearch::TextUnformatted("Trout");
                    ImSearch::TextUnformatted("Cod");
                    ImSearch::TextUnformatted("Haddock");
                    ImSearch::TextUnformatted("Shrimp");
                    ImSearch::TextUnformatted("Crab");
                    ImSearch::TextUnformatted("Lobster");
                    ImSearch::TextUnformatted("Sardines");
                    ImSearch::TextUnformatted("Mussels");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Boats"))
                {
                    ImSearch::TextUnformatted("Rowboat");
                    ImSearch::TextUnformatted("Sailboat");
                    ImSearch::TextUnformatted("Trawler");
                    ImSearch::TextUnformatted("Catamaran");
                    ImSearch::TextUnformatted("Kayak");
                    ImSearch::TextUnformatted("Dinghy");
                    ImSearch::TextUnformatted("Canoe");
                    ImSearch::TextUnformatted("Fishing Trawler");
                    ImSearch::TextUnformatted("Longliner");
                    ImSearch::TextUnformatted("Gillnetter");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            ImSearch::TreePop();
        }

        if (ImSearch::TreeNode("Technologies"))
        {
            if (ImSearch::TreeNode("Computers"))
            {
                if (ImSearch::TreeNode("Hardware"))
                {
                    ImSearch::TextUnformatted("CPU");
                    ImSearch::TextUnformatted("GPU");
                    ImSearch::TextUnformatted("RAM");
                    ImSearch::TextUnformatted("Motherboard");
                    ImSearch::TextUnformatted("SSD");
                    ImSearch::TextUnformatted("HDD");
                    ImSearch::TextUnformatted("Power Supply");
                    ImSearch::TextUnformatted("Cooler");
                    ImSearch::TextUnformatted("Case");
                    ImSearch::TextUnformatted("Network Card");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Software"))
                {
                    ImSearch::TextUnformatted("Operating System");
                    ImSearch::TextUnformatted("Web Browser");
                    ImSearch::TextUnformatted("Office Suite");
                    ImSearch::TextUnformatted("IDE");
                    ImSearch::TextUnformatted("Antivirus");
                    ImSearch::TextUnformatted("Drivers");
                    ImSearch::TextUnformatted("Database");
                    ImSearch::TextUnformatted("Virtual Machine");
                    ImSearch::TextUnformatted("Compiler");
                    ImSearch::TextUnformatted("Text Editor");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Networking"))
                {
                    ImSearch::TextUnformatted("Router");
                    ImSearch::TextUnformatted("Switch");
                    ImSearch::TextUnformatted("Firewall");
                    ImSearch::TextUnformatted("Modem");
                    ImSearch::TextUnformatted("Access Point");
                    ImSearch::TextUnformatted("Ethernet Cable");
                    ImSearch::TextUnformatted("Fiber Optic Cable");
                    ImSearch::TextUnformatted("VPN");
                    ImSearch::TextUnformatted("DNS");
                    ImSearch::TextUnformatted("DHCP");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            if (ImSearch::TreeNode("Vehicles"))
            {
                if (ImSearch::TreeNode("Land"))
                {
                    ImSearch::TextUnformatted("Car");
                    ImSearch::TextUnformatted("Truck");
                    ImSearch::TextUnformatted("Motorcycle");
                    ImSearch::TextUnformatted("Bicycle");
                    ImSearch::TextUnformatted("Bus");
                    ImSearch::TextUnformatted("Train");
                    ImSearch::TextUnformatted("Tram");
                    ImSearch::TextUnformatted("Tank");
                    ImSearch::TextUnformatted("ATV");
                    ImSearch::TextUnformatted("Segway");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Air"))
                {
                    ImSearch::TextUnformatted("Airplane");
                    ImSearch::TextUnformatted("Helicopter");
                    ImSearch::TextUnformatted("Drone");
                    ImSearch::TextUnformatted("Glider");
                    ImSearch::TextUnformatted("Hot Air Balloon");
                    ImSearch::TextUnformatted("Jet");
                    ImSearch::TextUnformatted("Blimp");
                    ImSearch::TextUnformatted("Autogyro");
                    ImSearch::TextUnformatted("Seaplane");
                    ImSearch::TextUnformatted("Hang Glider");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Sea"))
                {
                    ImSearch::TextUnformatted("Ship");
                    ImSearch::TextUnformatted("Boat");
                    ImSearch::TextUnformatted("Submarine");
                    ImSearch::TextUnformatted("Yacht");
                    ImSearch::TextUnformatted("Canoe");
                    ImSearch::TextUnformatted("Ferry");
                    ImSearch::TextUnformatted("Sailboat");
                    ImSearch::TextUnformatted("Tugboat");
                    ImSearch::TextUnformatted("Catamaran");
                    ImSearch::TextUnformatted("Dinghy");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            ImSearch::TreePop();
        }

        if (ImSearch::TreeNode("Nature"))
        {
            if (ImSearch::TreeNode("Animals"))
            {
                if (ImSearch::TreeNode("Mammals"))
                {
                    ImSearch::TextUnformatted("Lion");
                    ImSearch::TextUnformatted("Tiger");
                    ImSearch::TextUnformatted("Elephant");
                    ImSearch::TextUnformatted("Whale");
                    ImSearch::TextUnformatted("Dolphin");
                    ImSearch::TextUnformatted("Bat");
                    ImSearch::TextUnformatted("Kangaroo");
                    ImSearch::TextUnformatted("Human");
                    ImSearch::TextUnformatted("Bear");
                    ImSearch::TextUnformatted("Wolf");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Birds"))
                {
                    ImSearch::TextUnformatted("Eagle");
                    ImSearch::TextUnformatted("Sparrow");
                    ImSearch::TextUnformatted("Penguin");
                    ImSearch::TextUnformatted("Owl");
                    ImSearch::TextUnformatted("Parrot");
                    ImSearch::TextUnformatted("Flamingo");
                    ImSearch::TextUnformatted("Duck");
                    ImSearch::TextUnformatted("Goose");
                    ImSearch::TextUnformatted("Hawk");
                    ImSearch::TextUnformatted("Crow");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Reptiles"))
                {
                    ImSearch::TextUnformatted("Crocodile");
                    ImSearch::TextUnformatted("Snake");
                    ImSearch::TextUnformatted("Lizard");
                    ImSearch::TextUnformatted("Turtle");
                    ImSearch::TextUnformatted("Chameleon");
                    ImSearch::TextUnformatted("Gecko");
                    ImSearch::TextUnformatted("Alligator");
                    ImSearch::TextUnformatted("Komodo Dragon");
                    ImSearch::TextUnformatted("Iguana");
                    ImSearch::TextUnformatted("Rattlesnake");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }
        	if (ImSearch::TreeNode("Plants"))
            {
                if (ImSearch::TreeNode("Trees"))
                {
                    ImSearch::TextUnformatted("Oak");
                    ImSearch::TextUnformatted("Pine");
                    ImSearch::TextUnformatted("Maple");
                    ImSearch::TextUnformatted("Birch");
                    ImSearch::TextUnformatted("Cedar");
                    ImSearch::TextUnformatted("Redwood");
                    ImSearch::TextUnformatted("Palm");
                    ImSearch::TextUnformatted("Willow");
                    ImSearch::TextUnformatted("Spruce");
                    ImSearch::TextUnformatted("Cypress");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Flowers"))
                {
                    ImSearch::TextUnformatted("Rose");
                    ImSearch::TextUnformatted("Tulip");
                    ImSearch::TextUnformatted("Sunflower");
                    ImSearch::TextUnformatted("Daisy");
                    ImSearch::TextUnformatted("Orchid");
                    ImSearch::TextUnformatted("Lily");
                    ImSearch::TextUnformatted("Marigold");
                    ImSearch::TextUnformatted("Daffodil");
                    ImSearch::TextUnformatted("Chrysanthemum");
                    ImSearch::TextUnformatted("Iris");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Fungi"))
                {
                    ImSearch::TextUnformatted("Button Mushroom");
                    ImSearch::TextUnformatted("Shiitake");
                    ImSearch::TextUnformatted("Oyster Mushroom");
                    ImSearch::TextUnformatted("Morel");
                    ImSearch::TextUnformatted("Chanterelle");
                    ImSearch::TextUnformatted("Truffle");
                    ImSearch::TextUnformatted("Fly Agaric");
                    ImSearch::TextUnformatted("Porcini");
                    ImSearch::TextUnformatted("Puffball");
                    ImSearch::TextUnformatted("Enoki");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            ImSearch::TreePop();
        }
		if (ImSearch::TreeNode("Culinary"))
        {
            if (ImSearch::TreeNode("Ingredients"))
            {
                if (ImSearch::TreeNode("Spices"))
                {
                    ImSearch::TextUnformatted("Salt");
                    ImSearch::TextUnformatted("Pepper");
                    ImSearch::TextUnformatted("Paprika");
                    ImSearch::TextUnformatted("Cumin");
                    ImSearch::TextUnformatted("Turmeric");
                    ImSearch::TextUnformatted("Oregano");
                    ImSearch::TextUnformatted("Basil");
                    ImSearch::TextUnformatted("Thyme");
                    ImSearch::TextUnformatted("Cinnamon");
                    ImSearch::TextUnformatted("Nutmeg");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Produce"))
                {
                    ImSearch::TextUnformatted("Carrot");
                    ImSearch::TextUnformatted("Onion");
                    ImSearch::TextUnformatted("Garlic");
                    ImSearch::TextUnformatted("Pepper");
                    ImSearch::TextUnformatted("Tomato");
                    ImSearch::TextUnformatted("Lettuce");
                    ImSearch::TextUnformatted("Spinach");
                    ImSearch::TextUnformatted("Broccoli");
                    ImSearch::TextUnformatted("Eggplant");
                    ImSearch::TextUnformatted("Zucchini");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Proteins"))
                {
                    ImSearch::TextUnformatted("Chicken");
                    ImSearch::TextUnformatted("Beef");
                    ImSearch::TextUnformatted("Pork");
                    ImSearch::TextUnformatted("Tofu");
                    ImSearch::TextUnformatted("Lentils");
                    ImSearch::TextUnformatted("Fish");
                    ImSearch::TextUnformatted("Eggs");
                    ImSearch::TextUnformatted("Beans");
                    ImSearch::TextUnformatted("Lamb");
                    ImSearch::TextUnformatted("Turkey");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }
			if (ImSearch::TreeNode("Recipes"))
            {
                if (ImSearch::TreeNode("Soups"))
                {
                    ImSearch::TextUnformatted("Chicken Noodle Soup");
                    ImSearch::TextUnformatted("Tomato Soup");
                    ImSearch::TextUnformatted("Miso Soup");
                    ImSearch::TextUnformatted("Minestrone");
                    ImSearch::TextUnformatted("Clam Chowder");
                    ImSearch::TextUnformatted("Pho");
                    ImSearch::TextUnformatted("Ramen");
                    ImSearch::TextUnformatted("Gazpacho");
                    ImSearch::TextUnformatted("Pumpkin Soup");
                    ImSearch::TextUnformatted("Lentil Soup");
                    ImSearch::TreePop();
                }
                if (ImSearch::TreeNode("Desserts"))
                {
                    ImSearch::TextUnformatted("Chocolate Cake");
                    ImSearch::TextUnformatted("Apple Pie");
                    ImSearch::TextUnformatted("Ice Cream");
                    ImSearch::TextUnformatted("Brownies");
                    ImSearch::TextUnformatted("Cheesecake");
                    ImSearch::TextUnformatted("Pudding");
                    ImSearch::TextUnformatted("Tiramisu");
                    ImSearch::TextUnformatted("Crepes");
                    ImSearch::TextUnformatted("Cupcakes");
                    ImSearch::TextUnformatted("Macarons");
                    ImSearch::TreePop();
                }
                ImSearch::TreePop();
            }

            ImSearch::TreePop();
        }

		ImSearch::EndSearch();
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
}


#endif // #ifndef IMGUI_DISABLE
