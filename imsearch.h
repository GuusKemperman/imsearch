#pragma once

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

	using VTable = bool(*)(int mode, void* ptr1, void* ptr2);

	bool PushSearchable(const char* name);
	bool PushSearchable(const char* name, void* functor, VTable vTable);

	template<typename T>
	bool PushSearchable(const char* name, T&& functor)
	{
		T moveable{ static_cast<decltype(functor)>(functor) };
		return PushSearchable(
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

	void PopSearchable();
	void PopSearchable(void* functor, VTable vTable);

	template<typename T>
	void PopSearchable(T&& functor)
	{
		T moveable{ static_cast<decltype(functor)>(functor) };
		PopSearchable(
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

	// Shows the ImSearch demo window (add imsearch_demo.cpp to your sources!)
	void ShowDemoWindow(bool* p_open = nullptr);

	void TextUnformatted(const char* text);

	bool TreeNode(const char* text);
	void TreePop();
}

#endif // #ifndef IMGUI_DISABLE
