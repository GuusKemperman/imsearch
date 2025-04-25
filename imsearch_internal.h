#pragma once
#ifndef IMGUI_DISABLE
#include <cstddef>

namespace ImSearch
{
	namespace Internal
	{
		//-----------------------------------------------------------------------------
		// [SECTION] Testing
		//-----------------------------------------------------------------------------

		float GetScore(size_t index);

		size_t GetDisplayOrderEntry(size_t index);
	}
}

#endif // #ifndef IMGUI_DISABLE
