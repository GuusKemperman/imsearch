#pragma once
#ifndef IMGUI_DISABLE
#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <cctype>

#include <string>
#include <vector>
#include <memory>
#include <string>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <type_traits>
#include <numeric>
#include <limits>

namespace ImSearch
{
	using IndexT = std::uint32_t;


	//-----------------------------------------------------------------------------
	// [SECTION] Constants
	//-----------------------------------------------------------------------------

	// Anything below this score is not displayed to the user.
	constexpr float sCutOffStrength = .5f;

	constexpr IndexT sNullIndex = std::numeric_limits<IndexT>::max();

	//-----------------------------------------------------------------------------
	// [SECTION] Structs
	//-----------------------------------------------------------------------------

	struct StrView
	{
		StrView() = default;
		StrView(const std::string& str) :
			mData(str.data()),
			mSize(static_cast<IndexT>(str.size())) {}
		StrView(const char* data, IndexT size) :
			mData(data),
			mSize(size) {}

		const char* begin() const { return mData; }
		const char* end() const { return mData + mSize; }
		const char* data() const { return mData; }
		IndexT size() const { return mSize; }

		const char& operator[](IndexT i) const { IM_ASSERT(i < mSize); return mData[i]; }

		const char* mData{};
		IndexT mSize{};
	};

	// Here is why we are using VTables instead of just std::function:
	// std::function's SBO optimisation means it might consume
	// more memory than needed if the user has not captured anything,
	// and if the user is capturing more than fits in the SBO storage,
	// performance tanks due to the many heap allocations. Now,
	// the Callback is slower than using std::function, it makes
	// the implementation and API slightly more complicated, BUT, if we
	// were to release with std::function in the public API, I would not
	// be able to get rid of std::function without breaking forward
	// compatibility in the future. By keeping the concept of how functors
	// are stored abstracted away in the backend, it can be optimised in
	// the future without breaking API.
	struct Callback
	{
		Callback() = default;

		Callback(void* originalFunctor, ImSearch::Internal::VTable vTable);

		Callback(const Callback&) = delete;
		Callback(Callback&& other) noexcept;

		Callback& operator=(const Callback&) = delete;
		Callback& operator=(Callback&& other) noexcept;

		~Callback();

		operator bool() const { return mData != nullptr; }

		// PushSearchable
		bool operator()(const char* name) const;

		// PopSearchable
		void operator()() const;

		void ClearData();

		enum VTableModes
		{
			Invoke = 0,
			MoveConstruct = 1,
			Destruct = 2,
			GetSize = 3
		};

		ImSearch::Internal::VTable mVTable{};
		char* mData{};
	};

	struct Searchable
	{
		std::string mText{};

		IndexT mIndexOfFirstChild = sNullIndex;
		IndexT mIndexOfLastChild = sNullIndex;
		IndexT mIndexOfParent = sNullIndex;
		IndexT mIndexOfNextSibling = sNullIndex;
	};

	struct Input
	{
		std::vector<Searchable> mEntries{};
		std::vector<float> mBonuses{};
		std::string mUserQuery{};
	
		bool mReverseDisplayOrder{};
	};

	struct ReusableBuffers
	{
		std::vector<float> mScores{};
		std::vector<IndexT> mTempIndices{};
	};

	struct Output
	{
		std::vector<IndexT> mDisplayOrder{};
		static constexpr IndexT sDisplayEndFlag = static_cast<IndexT>(1) << static_cast<IndexT>(std::numeric_limits<IndexT>::digits - 1);
	
		std::string mToAppendOnAutoComplete{};
	};

	struct Result
	{
		Input mInput{};
		ReusableBuffers mBuffers{};
		Output mOutput{};
	};

	struct DisplayCallbacks
	{
		Callback mOnDisplayStart{};
		Callback mOnDisplayEnd{};
	};

	struct LocalContext
	{
		Input mInput{};

		std::vector<DisplayCallbacks> mDisplayCallbacks{};

		std::stack<IndexT> mPushStack{};
		Result mResult{};
		bool mHasSubmitted{};
	};

	struct ImSearchContext
	{
		std::unordered_map<ImGuiID, LocalContext> Contexts{};
		std::stack<std::reference_wrapper<LocalContext>> ContextStack{};
		std::unordered_map<std::string, std::string> mTokenisedStrings{};
	};

	bool operator==(const StrView& lhs, const StrView& rhs);
	bool operator==(const Searchable& lhs, const Searchable& rhs);
	bool operator==(const Input& lhs, const Input& rhs);

	//-----------------------------------------------------------------------------
	// [SECTION] Context
	//-----------------------------------------------------------------------------

	// Some helper functions for generalising error-reporting.
	LocalContext& GetLocalContext();
	ImSearch::ImSearchContext& GetImSearchContext();
	IndexT GetCurrentItem(LocalContext& context);
	
	// Is the context currently collecting submissions?
	// ImSearch does not store anything the programm is submitting if the user
	// is not actively searching, for performance and memory reasons.
	bool CanCollectSubmissions();

	void ReverseDisplayOrder();

	//-----------------------------------------------------------------------------
	// [SECTION] Testing
	//-----------------------------------------------------------------------------

	float GetScore(IndexT index);

	IndexT GetDisplayOrderEntry(IndexT index);
	
	IndexT GetNumItemsFilteredOut();

	//-----------------------------------------------------------------------------
	// [SECTION] Fuzzy Searching & String Functions
	//-----------------------------------------------------------------------------

	std::vector<std::string> SplitTokens(StrView s);

	std::string Join(const std::vector<std::string>& tokens);

	std::string MakeTokenisedString(StrView original);

	StrView GetMemoizedTokenisedString(const std::string& original);

	IndexT LevenshteinDistance(
		StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	float Ratio(StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	float PartialRatio(StrView s1,
		StrView s2,
		ReusableBuffers& buffers);

	// The function used internally to score strings
	float WeightedRatio(StrView s1,
		StrView s1Tokenised,
		StrView s2,
		StrView s2Tokenised,
		ReusableBuffers& buffers);
}

#endif // #ifndef IMGUI_DISABLE
