#pragma once

#ifndef IMGUI_DISABLE

namespace ImSearch
{
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
	}
}

template<typename T>
bool ImSearch::PushSearchable(const char* name, T&& functor)
{
	T moveable{ static_cast<decltype(functor)>(functor) };
	return Internal::PushSearchable(
		name,
		&moveable,
		+[](int mode, void* ptr1, void* ptr2)
		{
			switch (mode)
			{
			case 0: // Invoke
			{
				T* func = static_cast<T*>(ptr1);
				const char* name = static_cast<const char*>(ptr2);
				return (*func)(name);
			}
			case 1: // Move-construct
			{
				T* src = static_cast<T*>(ptr1);
				T* dst = static_cast<T*>(ptr2);
				new(dst)T(static_cast<T&&>(*src));
				return true;
			}
			case 2: // Destructor
			{
				T* src = static_cast<T*>(ptr1);
				src->~T();
				return true;
			}
			case 3: // Get size
			{
				int& ret = *static_cast<int*>(ptr1);
				ret = sizeof(T);
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
	T moveable{ static_cast<decltype(functor)>(functor) };
	Internal::PopSearchable(
		&moveable,
		+[](int mode, void* ptr1, void* ptr2)
		{
			switch (mode)
			{
			case 0: // Invoke
			{
				T* func = static_cast<T*>(ptr1);
				(*func)();
				return true;
			}
			case 1: // Move-construct
			{
				T* src = static_cast<T*>(ptr1);
				T* dst = static_cast<T*>(ptr2);
				new(dst)T(static_cast<T&&>(*src));
				return true;
			}
			case 2: // Destructor
			{
				T* src = static_cast<T*>(ptr1);
				src->~T();
				return true;
			}
			case 3: // Get size
			{
				int& ret = *static_cast<int*>(ptr1);
				ret = sizeof(T);
				return true;
			}
			default:
				return false;
			}
		});
}

#endif // #ifndef IMGUI_DISABLE
