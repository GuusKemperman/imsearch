#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE

typedef int ImSearchCol;                // -> enum ImSearchCol_
typedef int ImSearchFlags;				// -> enum ImSearchFlags_     // Flags: for BeginSearch()

enum ImSearchFlags_
{
	ImSearchFlags_None = 0,
	ImSearchFlags_NoTextHighlighting = 1 << 0
};

// Plot styling colors.
enum ImSearchCol_
{
	// item styling colors
	ImSearchCol_TextHighlighted, // All matching substrings are highlighted by default, and will be this colour
	ImSearchCol_TextHighlightedBg, // Background colour of all highlighted text
	ImSearchCol_COUNT
};

// Plot style structure
struct ImSearchStyle
{
	// style colors
	ImVec4  Colors[ImSearchCol_COUNT]; // Array of styling colors. Indexable with ImSearchCol_ enums.

	ImSearchStyle();
};

namespace ImSearch
{
	//-----------------------------------------------------------------------------
	// [SECTION] Contexts
	//-----------------------------------------------------------------------------

	struct ImSearchContext;

	// Creates a new ImSearch context. Call this after ImGui::CreateContext.
	ImSearchContext* CreateContext();

	// Destroys an ImSearch context. Call this before ImGui::DestroyContext. nullptr = destroy current context
	void DestroyContext(ImSearchContext* ctx = nullptr);

	// Returns the current context. nullptr if not context has been set.
	ImSearchContext* GetCurrentContext();

	// Sets the current context. 
	void SetCurrentContext(ImSearchContext* ctx);

	//-----------------------------------------------------------------------------
	// [SECTION] Begin/End Search contexts
	//-----------------------------------------------------------------------------

	// Starts a searching context. If this function returns true, EndSearch() MUST
	// be called! You are encouraged to use the following convention:
	//
	//	if (BeginSearch())
	//	{
	//		SearchBar();
	//
	//		ImSearch::SearchableItem("Hello world!",
	//			[](const char* str)
	//			{
	//				ImGui::Button(str);
	//			});
	//
	//		...
	//
	//		EndSearch();
	//	}
	//
	// See imsearch_demo.cpp for more examples.
	// 
	// Important notes:
	//
	// - BeginSearch must be unique to the current ImGui ID scope, having multiple
	//   calls to BeginSearch leads to ID collisions. If you need to avoid ID
	//   collisions, use ImGui::PushId
	bool BeginSearch(ImSearchFlags flags = 0);

	// Only call EndSearch() if BeginSearch() returns true! Typically called at the end
	// of an if statement conditioned on BeginSearch(). See example above.
	// Calls Submit, if Submit was not already explicitly called by the user.
	// See documentation for Submit below.
	void EndSearch();

	// ImSearch is free to invoke your callbacks anywhere between you submitting them,
	// and you calling EndSearch. Sometimes you need more control over when the callbacks
	// are invoked by ImSearch, which is what Submit is for. 
	//
	// For example, when you want the callbacks to be invoked within an ImGui child window:
	//
	//	if (ImSearch::BeginSearch())
	//	{
	//		ImSearch::SearchBar();
	//	
	//		if (ImGui::BeginChild("Submissions", {}, ImGuiChildFlags_Borders))
	//		{
	//			ImSearch::SearchableItem("Hello world!",
	//				[](const char* str)
	//				{
	//					ImGui::Button(str);
	//				});
	//	
	//			// Call Submit explicitly; all the callbacks
	//			// will be invoked through submit. If we
	//			// had waited for EndSearch to do this for us,
	//			// the callbacks would've been invoked after
	//			// ImGui::EndChild, leaving our searchables
	//			// to be displayed outside of the child window.
	//			ImSearch::Submit();
	//		} ImGui::EndChild();
	//	
	//		ImSearch::EndSearch();
	//	}
	void Submit();

	//-----------------------------------------------------------------------------
	// [SECTION] Submitting Searchables
	//-----------------------------------------------------------------------------

	// Add a searchable with a callback, wrapping some ImGui function calls, for example:
	// 
	//	ImSearch::SearchableItem("Hello world!",
	//		[](const char* str)
	//		{
	//			ImGui::Button(str);
	//		});
	//
	// Note that callbacks are called in order of relevancy, or maybe not even called at all,
	// if they are not relevant. ImSearch is free to invoke your callbacks anywhere between you
	// submitting them, and you calling EndSearch or Submit. The callback may have data
	// associated with (e.g., lambda captures). Make sure that your callback object
	// is not referencing anything that will be out of scope by then. For more information, see
	// 'How do callbacks work?' in imsearch_demo.cpp.
	//
	// Callback is an object or function pointer with a function of the form: void Func(const char* name).
	template<typename T>
	void SearchableItem(const char* name, T&& callback);

	// Push a searchable with a callback, wrapping some ImGui function calls.
	// If this function returns true, EndSearch() MUST
	// be called! You are encouraged to use the following convention:
	//
	// if (PushSearchable("Hello world!", [](const char* name) { return ImGui::TreeNode(name); })
	// {
	//		PopSearchable([](){ ImGui::TreePop(); });
	// }
	//
	// The provided callback function should return true if the 'children' should also be
	// displayed, similar to ImGui's TreeNodes.
	//
	// Callback is an object or function pointer with a function of the form: bool Func(const char* name).
	//
	// Note that callbacks are called in order of relevancy, or maybe not even called at all,
	// if they are not relevant. ImSearch is free to invoke your callbacks anywhere between you
	// submitting them, and you calling EndSearch or Submit. The callback may have data
	// associated with (e.g., lambda captures). Make sure that your callback object
	// is not referencing anything that will be out of scope by then. For more information, see
	// 'How do callbacks work?' in imsearch_demo.cpp.
	template<typename T>
	bool PushSearchable(const char* name, T&& callback);

	// Only call PopSearchable() if PushSearchable() returns true! Typically called at the end
	// of an if statement conditioned on PushSearchable(). See example above PushSearchable.
	void PopSearchable();

	// Only call PopSearchable() if PushSearchable() returns true! Typically called at the end
	// of an if statement conditioned on PushSearchable(). 
	// You can use a callback for wrapping all the ImGui calls needed for 'ending' your widget,
	// a common example being `ImGui::TreePop()`, see example above PushSearchable.
	//
	// Callback is an object or function pointer with a function of the form: void Func().
	template<typename T>
	void PopSearchable(T&& callback);

	//-----------------------------------------------------------------------------
	// [SECTION] Modifiers
	//-----------------------------------------------------------------------------

	// You can artificially increase the relevancy for items you think might
	// be more likely to be what the user is looking for.
	//
	// You could, for example, give priority to more commonly referenced functions:
	//
	//	if (ImSearch::PushSearchable(func.name, &DisplayFuncWidget))
	//	{
	//		float frequency = func.timesUsedInCodebase / gTotalFunctionsUsedInCodeBase;
	//		ImSearch::SetRelevancyBonus(frequency);
	//		ImSearch::PopSearchable();
	//	}
	//
	// ImSearch places no restrictions on the range of your provided bonus,
	// but keep in mind the 'default' relevancy of items, as scored by ImSearch,
	// is within the range of 0.0f to 1.0f. Normalization is encouraged, to avoid
	// your bonus dwarfing the similarity score of the text.
	void SetRelevancyBonus(float bonus);

	// You can add synonyms using the following syntax: 
	//
	//	if (ImSearch::PushSearchable("Function", selectableCallback))
	//	{
	//		ImSearch::AddSynonym("Method");
	//		ImSearch::AddSynonym("Procedure");
	//
	//		ImSearch::PopSearchable();
	//	}
	//
	void AddSynonym(const char* synonym);

	//-----------------------------------------------------------------------------
	// [SECTION] Searchbars
	//-----------------------------------------------------------------------------

	// Shows the default searchbar. Usually placed right after BeginSearch, or
	// right after calling Submit.
	void SearchBar(const char* hint = "Search");

	// API for setting the user query, the text that the user has typed
	// and is currently searching for. Used for making custom searchbars.
	void SetUserQuery(const char* query);

	// Will return the text that the user has typed
	// and is currently searching for.
	const char* GetUserQuery();

	//-----------------------------------------------------------------------------
	// [SECTION] Styling
	//-----------------------------------------------------------------------------

	// Like ImGui, all style colors are stored in indexable array in ImSearchStyle. 
	// You can permanently modify these values through GetStyle().Colors, or 
	// temporarily modify them with Push/Pop functions below.

	ImSearchStyle& GetStyle();

	ImU32 GetColorU32(ImSearchCol idx, float alpha_mul = 1.0f);
	const ImVec4& GetStyleColorVec4(ImSearchCol idx);

	// Use PushStyleX to temporarily modify your ImSearchStyle. The modification
	// will last until the matching call to PopStyleX. You MUST call a pop for
	// every push, otherwise you will leak memory! This behaves just like ImGui.

	// Temporarily modify a style color. Don't forget to call PopStyleColor!
	void PushStyleColor(ImSearchCol idx, ImU32 col);
	void PushStyleColor(ImSearchCol idx, const ImVec4& col);

	// Undo temporary style color modification(s). Undo multiple pushes at once by increasing count.
	void PopStyleColor(int count = 1);

	//-----------------------------------------------------------------------------
	// [SECTION] Demo
	//-----------------------------------------------------------------------------

	// Shows the ImSearch demo window (add imsearch_demo.cpp to your sources!)
	void ShowDemoWindow(bool* p_open = nullptr);

	//-----------------------------------------------------------------------------
	// [SECTION] Internal
	//-----------------------------------------------------------------------------

	// End of public API!
	// Starting from here until the end of the file,
	// forwards compatibility is not guaranteed!

	namespace Internal
	{
		using VTable = bool(*)(int mode, void* ptr1, void* ptr2);

		bool PushSearchable(const char* name, void* callback, VTable vTable);
		void PopSearchable(void* callback, VTable vTable);

		template<class T> struct remove_reference { typedef T type; };
		template<class T> struct remove_reference<T&> { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };
	}
}

template <typename T>
void ImSearch::SearchableItem(const char* name, T&& callback)
{
	using TNonRef = typename Internal::remove_reference<T>::type;

	struct CallbackWrapper
	{
		TNonRef mUserCallback;

		bool operator()(const char* name) const
		{
			(void)mUserCallback(name);
			return false;
		}
	};

	if (PushSearchable(name, CallbackWrapper{ static_cast<decltype(callback)>(callback) }))
	{
		PopSearchable();
	}
}

template<typename T>
bool ImSearch::PushSearchable(const char* name, T&& callback)
{
	using TNonRef = typename Internal::remove_reference<T>::type;
	TNonRef moveable{ static_cast<decltype(callback)>(callback) };
	return Internal::PushSearchable(
		name,
		&moveable,
		+[](int mode, void* ptr1, void* ptr2) -> bool
		{
			switch (mode)
			{
			case 0: // Invoke
			{
				TNonRef* func = static_cast<TNonRef*>(ptr1);
				const char* nameArg = static_cast<const char*>(ptr2);
				return (*func)(nameArg);
			}
			case 1: // Move-construct
			{
				TNonRef* src = static_cast<TNonRef*>(ptr1);
				TNonRef* dst = static_cast<TNonRef*>(ptr2);
				new(dst)TNonRef(static_cast<TNonRef&&>(*src));
				return true;
			}
			case 2: // Destructor
			{
				TNonRef* src = static_cast<TNonRef*>(ptr1);
				src->~TNonRef();
				return true;
			}
			case 3: // Get size
			{
				int& ret = *static_cast<int*>(ptr1);
				ret = sizeof(TNonRef);
				return true;
			}
			default:
				return false;
			}
		});
}

template<typename T>
void ImSearch::PopSearchable(T&& callback)
{
	using TNonRef = typename Internal::remove_reference<T>::type;
	TNonRef moveable{ static_cast<decltype(callback)>(callback) };
	Internal::PopSearchable(
		&moveable,
		+[](int mode, void* ptr1, void* ptr2)
		{
			switch (mode)
			{
			case 0: // Invoke
			{
				TNonRef* func = static_cast<TNonRef*>(ptr1);
				(*func)();
				return true;
			}
			case 1: // Move-construct
			{
				TNonRef* src = static_cast<TNonRef*>(ptr1);
				TNonRef* dst = static_cast<TNonRef*>(ptr2);
				new(dst)TNonRef(static_cast<TNonRef&&>(*src));
				return true;
			}
			case 2: // Destructor
			{
				TNonRef* src = static_cast<TNonRef*>(ptr1);
				src->~TNonRef();
				return true;
			}
			case 3: // Get size
			{
				int& ret = *static_cast<int*>(ptr1);
				ret = sizeof(TNonRef);
				return true;
			}
			default:
				return false;
			}
		});
}

#endif // #ifndef IMGUI_DISABLE
