#pragma once

#ifndef IMGUI_DISABLE

namespace ImSearch
{
	struct ImSearchContext;

	// Creates a new ImSearch context. Call this after ImGui::CreateContext.
	ImSearchContext* CreateContext();

	// Destroys an ImSearch context. Call this before ImGui::DestroyContext. nullptr = destroy current context
	void DestroyContext(ImSearchContext* ctx = nullptr);

	// Returns the current context. nullptr if not context has been set.
	ImSearchContext* GetCurrentContext();

	// Sets the current context. 
	void SetCurrentContext(ImSearchContext* ctx);

	/**
	 * \brief Requires End() to be called if returns true
	 */
	bool BeginSearch();

	void SearchBar(const char* hint = "Search");

	/**
	 * \brief Calls the submitted display functions, in the order of relevancy to the user.
	 */
	void Submit();

	/**
	 * \brief Calls Submit, if that was not called already, and blocks further submissions.
	 */
	void EndSearch();

	using VTable = bool(*)(int mode, void* ptr1, void* ptr2);

	/** TODO: Improve documentation
	 * Functor is a callback object/ptr for a function of the form: bool Func(const char* name). The function
	 * should return true if the 'children' should also be displayed.
	 * The functor may have data associated with (e.g., lambda captures), but must be move-constructable. 
	 */
	template<typename T>
	bool PushSearchable(const char* name, T&& functor);

	void PopSearchable();

	template<typename T>
	void PopSearchable(T&& functor);

	void SetUserQuery(const char* query);

	const char* GetUserQuery();

	// Shows the ImSearch demo window (add imsearch_demo.cpp to your sources!)
	void ShowDemoWindow(bool* p_open = nullptr);

	namespace Internal // Forwards compatibility not guaranteed!
	{
		bool PushSearchable(const char* name, void* functor, VTable vTable);
		void PopSearchable(void* functor, VTable vTable);

		template<class T> struct remove_reference { typedef T type; };
		template<class T> struct remove_reference<T&> { typedef T type; };
		template<class T> struct remove_reference<T&&> { typedef T type; };
	}
}

template<typename T>
bool ImSearch::PushSearchable(const char* name, T&& functor)
{
	using TNonRef = typename Internal::remove_reference<T>::type;
	TNonRef moveable{ static_cast<decltype(functor)>(functor) };
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
				const char* name = static_cast<const char*>(ptr2);
				return (*func)(name);
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
void ImSearch::PopSearchable(T&& functor)
{
	using TNonRef = typename Internal::remove_reference<T>::type;
	TNonRef moveable{ static_cast<decltype(functor)>(functor) };
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
