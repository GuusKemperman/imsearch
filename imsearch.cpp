#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imsearch_internal.h"
#include "imgui.h"

#include <memory>
#include <vector>
#include <string>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <type_traits>
#include <numeric>
#include <limits>
#include <cctype>

namespace
{
	using IndexT = std::uint32_t;

	constexpr IndexT sNullIndex = std::numeric_limits<IndexT>::max();

	// Here is why we are using VTables instead of just std::function:
	// std::function's SBO optimisation means it might consume
	// more memory than needed if the user has not captured anything,
	// and if the user is capturing more than fits in the SBO storage,
	// performance tanks due to the many heap allocations. Now,
	// the VTableFunctor is slower than using std::function, it makes
	// the implementation and API slightly more complicated, BUT, if we
	// were to release with std::function in the public API, I would not
	// be able to get rid of std::function without breaking forward
	// compatibility in the future. By keeping the concept of how functors
	// are stored abstracted away in the backend, it can be optimised in
	// the future without breaking API.
	struct VTableFunctor
	{
		VTableFunctor() = default;

		VTableFunctor(void* originalFunctor, ImSearch::Internal::VTable vTable);

		VTableFunctor(const VTableFunctor&) = delete;
		VTableFunctor(VTableFunctor&& other) noexcept;

		VTableFunctor& operator=(const VTableFunctor&) = delete;
		VTableFunctor& operator=(VTableFunctor&& other) noexcept;

		~VTableFunctor();

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
		std::string mUserQuery{};
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
	};

	struct Result
	{
		Input mInput{};
		ReusableBuffers mBuffers{};
		Output mOutput{};
	};

	struct DisplayCallbacks
	{
		VTableFunctor mOnDisplayStart{};
		VTableFunctor mOnDisplayEnd{};
	};

	struct LocalContext
	{
		Input mInput{};

		std::vector<DisplayCallbacks> mDisplayCallbacks{};

		std::stack<IndexT> mPushStack{};
		Result mResult{};
		bool mHasSubmitted{};
	};
}

namespace ImSearch
{
	struct ImSearchContext
	{
		std::unordered_map<ImGuiID, LocalContext> Contexts{};
		std::stack<std::reference_wrapper<LocalContext>> ContextStack{};
		std::unordered_map<std::string, std::string> mPreprocessedStrings{};
	};
}

namespace
{
	ImSearch::ImSearchContext* sContext{};

	// Anything below this score is not displayed to the user.
	constexpr float sCutOffStrength = .3f;

	LocalContext& GetLocalContext();
	ImSearch::ImSearchContext& GetImSearchContext();

	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput);
	void BringResultUpToDate(Result& result);
	void DisplayToUser(const LocalContext& context, const Result& result);
}

ImSearch::ImSearchContext* ImSearch::CreateContext()
{
	ImSearchContext* ctx = IM_NEW(ImSearchContext)();
	if (sContext == nullptr)
	{
		SetCurrentContext(ctx);
	}
	return ctx;
}

void ImSearch::DestroyContext(ImSearchContext* ctx)
{
	if (ctx == nullptr)
	{
		ctx = sContext;
	}

	if (sContext == ctx)
	{
		SetCurrentContext(nullptr);
	}
	IM_DELETE(ctx);
}

ImSearch::ImSearchContext* ImSearch::GetCurrentContext()
{
	return sContext;
}

void ImSearch::SetCurrentContext(ImSearchContext* ctx)
{
	sContext = ctx;
}

bool ImSearch::BeginSearch()
{
	const ImGuiID imId = ImGui::GetID("Search");
	ImGui::PushID(static_cast<int>(imId));

	ImSearchContext& context = GetImSearchContext();
	LocalContext& localContext = context.Contexts[imId];
	context.ContextStack.emplace(localContext);

	localContext.mHasSubmitted = false;

	return true;
}

void ImSearch::SearchBar(const char* hint)
{
	LocalContext& context = GetLocalContext();

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);

	std::string& str = context.mInput.mUserQuery;
	ImGui::InputTextWithHint("##SearchBar",
		hint,
		const_cast<char*>(str.c_str()),
		str.capacity() + 1,
		ImGuiInputTextFlags_CallbackResize,
		+[](ImGuiInputTextCallbackData* data) -> int
		{
			std::string* str = static_cast<std::string*>(data->UserData);
			if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				// Resize string callback
				// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
				IM_ASSERT(data->Buf == str->c_str());
				str->resize(data->BufTextLen);
				data->Buf = const_cast<char*>(str->c_str());
			}
			return 0;
		},
		&str);

	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
		str.clear();
	}
}

void ImSearch::Submit()
{
	LocalContext& context = GetLocalContext();
	Result& lastValidResult = context.mResult;

	if (!IsResultUpToDate(lastValidResult, context.mInput))
	{
		lastValidResult.mInput = context.mInput;
		BringResultUpToDate(lastValidResult);
	}

	DisplayToUser(context, lastValidResult);

	context.mInput.mEntries.clear();
	context.mDisplayCallbacks.clear();
	IM_ASSERT(context.mPushStack.empty() && "There were more calls to PushSearchable than to PopSearchable");
}

void ImSearch::EndSearch()
{
	LocalContext& localContext = GetLocalContext();

	if (!localContext.mHasSubmitted)
	{
		Submit();
	}

	ImGui::PopID();

	ImSearch::ImSearchContext& context = GetImSearchContext();
	context.ContextStack.pop();
}

bool ImSearch::Internal::PushSearchable(const char* name, void* functor, VTable vTable)
{
	LocalContext& context = GetLocalContext();

	IM_ASSERT(!context.mHasSubmitted && "Tried calling PushSearchable after EndSearch or Submit");

	context.mInput.mEntries.emplace_back();
	Searchable& searchable = context.mInput.mEntries.back();
	searchable.mText = std::string{ name };

	context.mDisplayCallbacks.emplace_back();
	if (functor != nullptr
		&& vTable != nullptr)
	{
		context.mDisplayCallbacks.back().mOnDisplayStart = VTableFunctor{ functor, vTable };
	}

	const IndexT currentIndex = static_cast<IndexT>(context.mInput.mEntries.size() - static_cast<size_t>(1ull));
	
	if (!context.mPushStack.empty())
	{
		const IndexT parentIndex = context.mPushStack.top();
		searchable.mIndexOfParent = parentIndex;

		Searchable& parent = context.mInput.mEntries[parentIndex];

		if (parent.mIndexOfFirstChild == sNullIndex)
		{
			parent.mIndexOfFirstChild = currentIndex;
			parent.mIndexOfLastChild = currentIndex;
		}
		else
		{
			const IndexT prevIndex = parent.mIndexOfLastChild;
			parent.mIndexOfLastChild = currentIndex;
			IM_ASSERT(prevIndex != sNullIndex);

			Searchable& prevSibling = context.mInput.mEntries[prevIndex];
			prevSibling.mIndexOfNextSibling = currentIndex;
		}
	}

	context.mPushStack.emplace(currentIndex);
	return true;
}

void ImSearch::PopSearchable()
{
	Internal::PopSearchable(nullptr, nullptr);
}

void ImSearch::SetUserQuery(const char* query)
{
	LocalContext& context = GetLocalContext();
	context.mInput.mUserQuery = std::string{ query };
}

const char* ImSearch::GetUserQuery()
{
	ImSearch::ImSearchContext& context = GetImSearchContext();

	if (context.ContextStack.empty())
	{
		return nullptr;
	}
	return context.ContextStack.top().get().mInput.mUserQuery.c_str();
}

void ImSearch::Internal::PopSearchable(void* functor, VTable vTable)
{
	LocalContext& context = GetLocalContext();

	IM_ASSERT(!context.mHasSubmitted && "Tried calling PopSearchable after EndSearch or Submit");

	const size_t indexOfCurrentCategory = context.mPushStack.top();

	if (functor != nullptr
		&& vTable != nullptr)
	{
		context.mDisplayCallbacks[indexOfCurrentCategory].mOnDisplayEnd = VTableFunctor{ functor, vTable };
	}

	context.mPushStack.pop();
}

namespace
{
	VTableFunctor::VTableFunctor(void* originalFunctor, ImSearch::Internal::VTable vTable) :
		mVTable(vTable)
	{
		if (mVTable == nullptr)
		{
			return;
		}

		int size;
		vTable(VTableModes::GetSize, &size, nullptr);
		mData = static_cast<char*>(ImGui::MemAlloc(static_cast<size_t>(size)));
		IM_ASSERT(mData != nullptr);
		vTable(VTableModes::MoveConstruct, originalFunctor, mData);
	}

	VTableFunctor::VTableFunctor(VTableFunctor&& other) noexcept:
		mVTable(other.mVTable),
		mData(other.mData)
	{
		other.mVTable = nullptr;
		other.mData = nullptr;
	}

	VTableFunctor& VTableFunctor::operator=(VTableFunctor&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		ClearData();

		mVTable = other.mVTable;
		mData = other.mData;

		other.mVTable = nullptr;
		other.mData = nullptr;

		return *this;
	}

	VTableFunctor::~VTableFunctor()
	{
		ClearData();
	}

	bool VTableFunctor::operator()(const char* name) const
	{
		return mVTable(VTableModes::Invoke, mData, const_cast<char*>(name));
	}

	void VTableFunctor::operator()() const
	{
		mVTable(VTableModes::Invoke, mData, nullptr);
	}

	void VTableFunctor::ClearData()
	{
		if (mData == nullptr)
		{
			return;
		}

		mVTable(VTableModes::Destruct, mData, nullptr);

		ImGui::MemFree(mData);
		mData = nullptr;
	}

	LocalContext& GetLocalContext()
	{
		ImSearch::ImSearchContext& ImSearchContext = GetImSearchContext();
		IM_ASSERT(!ImSearchContext.ContextStack.empty() && "Not currently in between a ImSearch::BeginSearch and ImSearch::EndSearch");
		return ImSearchContext.ContextStack.top();
	}

	ImSearch::ImSearchContext& GetImSearchContext()
	{
		ImSearch::ImSearchContext* context = ImSearch::GetCurrentContext();
		IM_ASSERT(sContext != nullptr && "An ImSearchContext has not been created, see ImSearch::CreateContext");
		return *context;
	}

	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput)
	{
		const Input& oldInput = oldResult.mInput;

		if (oldInput.mUserQuery != currentInput.mUserQuery
			|| oldInput.mEntries.size() != currentInput.mEntries.size())
		{
			return false;
		}

		// C++11 didn't have nice algorithms for comparing ranges :(
		for (size_t i = 0; i < oldInput.mEntries.size(); i++)
		{
			const Searchable& oldEntry = oldInput.mEntries[i];
			const Searchable& newEntry = currentInput.mEntries[i];
			if (oldEntry.mText != newEntry.mText
				|| oldEntry.mIndexOfFirstChild != newEntry.mIndexOfFirstChild
				|| oldEntry.mIndexOfLastChild != newEntry.mIndexOfLastChild
				|| oldEntry.mIndexOfParent != newEntry.mIndexOfParent
				|| oldEntry.mIndexOfNextSibling != newEntry.mIndexOfNextSibling)
			{
				return false;
			}
		}
		return true;
	}

	std::vector<std::string> TokeniseAndSort(const std::string& s)
	{
		std::vector<std::string> tokens{};
		std::string current{};
		for (char c : s)
		{
			if (std::isalnum(static_cast<unsigned char>(c)))
			{
				current += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				continue;
			}

			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}

		if (!current.empty())
		{
			tokens.push_back(current);
		}
		std::sort(tokens.begin(), tokens.end());
		return tokens;
	}

	std::string Join(const std::vector<std::string>& tokens)
	{
		if (tokens.empty())
		{
			return {};
		}

		std::string res = tokens[0];
		for (size_t i = 1; i < tokens.size(); i++)
		{
			res += ' ' + tokens[i];
		}
		return res;
	}

	int LevenshteinDistance(const std::string& s1, const std::string& s2, ReusableBuffers& buffers)
	{
		const int matWidth = static_cast<int>(s1.size()) + 1;
		const int matHeight = static_cast<int>(s2.size()) + 1;

		std::vector<IndexT>& dp = buffers.mTempIndices;
		buffers.mTempIndices.resize(matWidth * matHeight);

		for (int x = 0; x < matWidth; x++)
		{
			dp[x] = x;
			
		}
		for (int y = 0; y < matHeight; y++)
		{
			dp[y * matWidth] = y;
		}

		for (int x = 1; x < matWidth; x++)
		{
			for (int y = 1; y < matHeight; y++)
			{
				const int cost = (s1[x - 1] == s2[y - 1]) ? 0 : 1;

				dp[x + y * matWidth] = 
					std::min({
						dp[(x - 1) + y * matWidth] + 1,
						dp[x + (y - 1) * matWidth] + 1,
						dp[(x - 1) + (y - 1) * matWidth] + cost
					});
			}
		}

		return static_cast<int>(dp.back());
	}

	int PartialRatio(const std::string& s1, const std::string& s2, ReusableBuffers& buffers)
	{
		if (s1.empty()
			|| s2.empty())
		{
			return 0;
		}

		const std::string& shorter = s1.size() <= s2.size() ? s1 : s2;
		const std::string& longer = (s1.size() <= s2.size()) ? s2 : s1;

		const int shorterSize = static_cast<int>(shorter.size());
		const int longerSize = static_cast<int>(longer.size());

		int max_ratio = 0;
		for (int i = 0; i <= longerSize - shorterSize; i++)
		{
			const std::string substring = longer.substr(i, shorterSize);
			const int distance = LevenshteinDistance(shorter, substring, buffers);
			const int ratio = static_cast<int>((1.0 - static_cast<double>(distance) / shorterSize) * 100);

			max_ratio = std::max(max_ratio, ratio);

			if (max_ratio == 100)
			{
				break;
			}
		}
		return max_ratio;
	}

	std::string MakePreprocessedString(const std::string& original)
	{
		const std::vector<std::string> targetTokens = TokeniseAndSort(original);
		std::string processedTarget = Join(targetTokens);
		return processedTarget;
	}

	const std::string& GetPreprocessedString(const std::string& original)
	{
		ImSearch::ImSearchContext& context = GetImSearchContext();
		auto& preprocessedStrings = context.mPreprocessedStrings;

		auto it = context.mPreprocessedStrings.find(original);

		if (it == preprocessedStrings.end())
		{
			it = preprocessedStrings.emplace(original, MakePreprocessedString(original)).first;
		}

		return it->second;
	}

	int PartialTokenSortRatio(const std::string& processedUserQuery, const std::string& target, ReusableBuffers& buffers)
	{
		return PartialRatio(processedUserQuery, GetPreprocessedString(target), buffers);
	}

	void AssignInitialScores(const Input& input, ReusableBuffers& buffers)
	{
		buffers.mScores.clear();
		buffers.mScores.resize(input.mEntries.size());

		if (input.mUserQuery.empty())
		{
			std::fill(buffers.mScores.begin(), buffers.mScores.end(), 1.0f);
			return;
		}

		const std::string& processedQuery = MakePreprocessedString(input.mUserQuery);

		for (IndexT i = 0; i < static_cast<IndexT>(input.mEntries.size()); i++)
		{
			const std::string& text = input.mEntries[i].mText;

			const int ratio = PartialTokenSortRatio(processedQuery, text, buffers);

			const float score = static_cast<float>(ratio) / 100.0f;
			buffers.mScores[i] = score;
		}
	}

	void PropagateScoreToChildren(const Input& input, ReusableBuffers& buffers)
	{
		// Each node can only be the child of ONE parent.
		// Children can only be submitted AFTER their parent.
		// When iterating over the nodes, we will always
		// encounter a parent before a child.
		for (IndexT parentIndex = 0; parentIndex < static_cast<IndexT>(input.mEntries.size()); parentIndex++)
		{
			const Searchable& parent = input.mEntries[parentIndex];
			const float parentScore = buffers.mScores[parentIndex];

			for (IndexT childIndex = parent.mIndexOfFirstChild;
				childIndex != sNullIndex;
				childIndex = input.mEntries[childIndex].mIndexOfNextSibling)
			{
				float& childScore = buffers.mScores[childIndex];
				childScore = std::max(childScore, parentScore);
			}
		}
	}

	void PropagateScoreToParents(const Input& input, ReusableBuffers& buffers)
	{
		// Children can only be submitted AFTER their parent.
		// When iterating over the entries in reverse, we will
		// always reach a node's child before the node itself.
		for (IndexT childIndex = static_cast<IndexT>(input.mEntries.size()); childIndex --> 0;)
		{
			const IndexT parentIndex = input.mEntries[childIndex].mIndexOfParent;

			if (parentIndex == sNullIndex)
			{
				continue;
			}

			const float childScore = buffers.mScores[childIndex];
			float& parentScore = buffers.mScores[parentIndex];
			parentScore = std::max(parentScore, childScore);
		}
	}

	void AppendToDisplayOrder(const Input& input, 
		ReusableBuffers& buffers, 
		IndexT startInIndicesBuffer,
		IndexT endInIndicesBuffer,
		Output& output)
	{
		std::stable_sort(buffers.mTempIndices.begin() + startInIndicesBuffer,
			buffers.mTempIndices.begin() + endInIndicesBuffer,
			[&](IndexT lhsIndex, IndexT rhsIndex) -> bool
			{
				const float lhsScore = buffers.mScores[lhsIndex];
				const float rhsScore = buffers.mScores[rhsIndex];

				return lhsScore > rhsScore;
			});

		for (IndexT indexInIndicesBuffer = startInIndicesBuffer; indexInIndicesBuffer < endInIndicesBuffer; indexInIndicesBuffer++)
		{
			IndexT searchableIndex = buffers.mTempIndices[indexInIndicesBuffer];

			const IndexT nextStartInIndicesBuffer = static_cast<IndexT>(buffers.mTempIndices.size());

			output.mDisplayOrder.emplace_back(searchableIndex);

			const Searchable& searchable = input.mEntries[searchableIndex];
			for (IndexT childIndex = searchable.mIndexOfFirstChild;
				childIndex != sNullIndex;
				childIndex = input.mEntries[childIndex].mIndexOfNextSibling)
			{
				if (buffers.mScores[childIndex] >= sCutOffStrength)
				{
					buffers.mTempIndices.emplace_back(childIndex);
				}
			}

			const IndexT nextEndInIndicesBuffer = static_cast<IndexT>(buffers.mTempIndices.size());

			AppendToDisplayOrder(input, 
				buffers, 
				nextStartInIndicesBuffer,
				nextEndInIndicesBuffer, 
				output);

			output.mDisplayOrder.emplace_back(searchableIndex | Output::sDisplayEndFlag);
		}
	}

	void GenerateDisplayOrder(const Input& input, ReusableBuffers& buffers, Output& output)
	{
		output.mDisplayOrder.clear();
		buffers.mTempIndices.clear();

		for (IndexT i = 0; i < static_cast<IndexT>(input.mEntries.size()); i++)
		{
			if (input.mEntries[i].mIndexOfParent == sNullIndex
				&& buffers.mScores[i] >= sCutOffStrength)
			{
				buffers.mTempIndices.emplace_back(i);
			}
		}

		AppendToDisplayOrder(input, 
			buffers,
			0,
			static_cast<IndexT>(buffers.mTempIndices.size()),
			output);
	}

	void BringResultUpToDate(Result& result)
	{
		AssignInitialScores(result.mInput, result.mBuffers);
		PropagateScoreToChildren(result.mInput, result.mBuffers);
		PropagateScoreToParents(result.mInput, result.mBuffers);
		GenerateDisplayOrder(result.mInput, result.mBuffers, result.mOutput);
	}

	void DisplayToUser(const LocalContext& context, const Result& result)
	{
		const bool isUserSearching = !result.mInput.mUserQuery.empty();
		ImGui::PushID(isUserSearching);

		const std::vector<IndexT>& displayOrder = result.mOutput.mDisplayOrder;

		for (auto it = displayOrder.begin(); it != displayOrder.end(); ++it)
		{
			const IndexT indexAndFlag = *it;
			const IndexT index = indexAndFlag & ~Output::sDisplayEndFlag;
			const IndexT isEnd = indexAndFlag & Output::sDisplayEndFlag;

			const Searchable& searchable = result.mInput.mEntries[index];
			const DisplayCallbacks& callbacks = context.mDisplayCallbacks[index];

			if (isEnd)
			{
				if (callbacks.mOnDisplayEnd)
				{
					callbacks.mOnDisplayEnd();
				}
				continue;
			}

			if (!callbacks.mOnDisplayStart)
			{
				continue;
			}

			if (isUserSearching)
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}

			if (callbacks.mOnDisplayStart(searchable.mText.c_str()))
			{
				continue;
			}

			// The user start function was called, but returned false.
			// We need to avoid displaying this searchable's children,
			// and make sure we call the corresponding mOnDisplayEnd
			it = std::find(it + 1, displayOrder.end(), index | Output::sDisplayEndFlag);
			IM_ASSERT(it != displayOrder.end());
		}

		ImGui::PopID();
	}
}

float ImSearch::Internal::GetScore(size_t index)
{
	LocalContext& context = GetLocalContext();
	auto& scores = context.mResult.mBuffers.mScores;

	if (index < 0
		|| index >= scores.size())
	{
		return -1.0f;
	}
	return scores[index];
}

size_t ImSearch::Internal::GetDisplayOrderEntry(size_t index)
{
	LocalContext& context = GetLocalContext();
	auto& displayOrder = context.mResult.mOutput.mDisplayOrder;

	if (index < 0
		|| index >= displayOrder.size())
	{
		return std::numeric_limits<size_t>::max();
	}
	return displayOrder[index];
}

#endif // #ifndef IMGUI_DISABLE
