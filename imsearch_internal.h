#pragma once
#ifndef IMGUI_DISABLE
#include <cstddef>
#include <cstdint>
#include <imgui.h>
#include <string>
#include <vector>

namespace ImSearch
{
	using IndexT = std::uint32_t;
	constexpr IndexT sNullIndex = std::numeric_limits<IndexT>::max();

	struct StrView
	{
		StrView() = default;
		StrView(const std::string& str) :
			mData(str.data()),
			mSize(static_cast<IndexT>(str.size())) {}
		StrView(const char* data, IndexT size) :
			mData(data),
			mSize(size) {}

		const char* data() const { return mData; }
		IndexT size() const { return mSize; }

		const char& operator[](IndexT i) const { IM_ASSERT(i < mSize); return mData[i]; }

		const char* mData{};
		IndexT mSize{};
	};

	namespace Internal
	{
		//-----------------------------------------------------------------------------
		// [SECTION] Testing
		//-----------------------------------------------------------------------------

		float GetScore(size_t index);

		size_t GetDisplayOrderEntry(size_t index);
	}

	struct ReusableBuffers
	{
		std::vector<float> mScores{};
		std::vector<IndexT> mTempIndices{};
	};

	int LevenshteinDistance(
		StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	float Ratio(StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	float PartialRatio(StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	float WRatio(StrView s1,
		StrView s2,
		ReusableBuffers& buffers);
}

#endif // #ifndef IMGUI_DISABLE
