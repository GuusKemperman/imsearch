
#include <functional>
#include <memory>
#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imsearch_internal.h"
#include "imgui.h"

#include <vector>
#include <string>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <type_traits>
#include <numeric>
#include <limits>

namespace
{
	using IndexT = std::uint32_t;

	constexpr IndexT sNullIndex = std::numeric_limits<IndexT>::max();

	enum VTableModes
	{
		Invoke = 0,
		MoveConstruct = 1,
		Destruct = 2,
		GetSize = 3
	};

	struct Searchable
	{
		std::string mText{};

		std::function<bool(const char*)> mOnDisplayStart{};
		std::function<void()> mOnDisplayEnd{};

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
		std::vector<IndexT> mIndicesToAddToDisplayOrder{};
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

	struct SearchContext
	{
		Input mInput{};

		std::stack<IndexT> mPushStack{};
		Result mResult{};
	};
	std::unordered_map<ImGuiID, SearchContext> sContexts{};
	std::stack<std::reference_wrapper<SearchContext>> sContextStack{};

	// Anything below this score is not displayed to the user.
	constexpr float sCutOffStrength = .3f;

	constexpr const char* sDefaultLabel = "##SearchBar";
	constexpr const char* sDefaultHint = "Search";

	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput);
	void BringResultUpToDate(Result& result);
	void DisplayToUser(const Result& result);
}

void ImSearch::BeginSearch()
{
	const ImGuiID imId = ImGui::GetID("Search");
	ImGui::PushID(static_cast<int>(imId));

	sContextStack.emplace(sContexts[imId]);
	SearchContext& context = sContextStack.top();

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);

	std::string& str = context.mInput.mUserQuery;
	ImGui::InputTextWithHint(sDefaultLabel,
		sDefaultHint,
		const_cast<char*>(str.c_str()),
		str.capacity() + 1,
		ImGuiInputTextFlags_CallbackResize,
		+[](ImGuiInputTextCallbackData* data) -> int
		{
			std::string* str = (std::string*)data->UserData;
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
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void ImSearch::EndSearch()
{
	SearchContext& context = sContextStack.top();
	Result& lastValidResult = context.mResult;

	if (!IsResultUpToDate(lastValidResult, context.mInput))
	{
		lastValidResult.mInput = context.mInput;
		BringResultUpToDate(lastValidResult);
	}

	DisplayToUser(lastValidResult);

	ImGui::PopID();

	context.mInput.mEntries.clear();
	IM_ASSERT(context.mPushStack.empty() && "There were more calls to PushSearchable than to PopSearchable");

	sContextStack.pop();
}

bool ImSearch::PushSearchable(const char* name)
{
	return PushSearchable(name, nullptr, nullptr);
}

bool ImSearch::PushSearchable(const char* name, void* functor, VTable vTable)
{
	SearchContext& context = sContextStack.top();

	context.mInput.mEntries.emplace_back();
	Searchable& searchable = context.mInput.mEntries.back();
	searchable.mText = std::string{ name };

	if (functor != nullptr
		&& vTable != nullptr)
	{
		int size;
		vTable(VTableModes::GetSize, &size, nullptr);

		struct OwningFunctor
		{
			~OwningFunctor()
			{
				table(VTableModes::Destruct, memory.get(), nullptr);
			}
			VTable table{};
			std::unique_ptr<char[]> memory{};
		};

		std::shared_ptr<OwningFunctor> owningFunctor{ new OwningFunctor };
		owningFunctor->table = vTable;
		owningFunctor->memory = std::unique_ptr<char[]>{ new char[size] };
		vTable(VTableModes::MoveConstruct, functor, owningFunctor->memory.get());

		searchable.mOnDisplayStart =
			[data = owningFunctor](const char* name) -> bool
			{
				return data->table(VTableModes::Invoke, data->memory.get(), const_cast<char*>(name));
			};
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
	PopSearchable(nullptr, nullptr);
}

void ImSearch::PopSearchable(void* functor, VTable vTable)
{
	SearchContext& context = sContextStack.top();
	const size_t indexOfCurrentCategory = context.mPushStack.top();

	if (functor != nullptr
		&& vTable != nullptr)
	{
		int size;
		vTable(VTableModes::GetSize, &size, nullptr);

		struct OwningFunctor
		{
			~OwningFunctor()
			{
				table(VTableModes::Destruct, memory.get(), nullptr);
			}
			VTable table{};
			std::unique_ptr<char[]> memory{};
		};

		std::shared_ptr<OwningFunctor> owningFunctor{ new OwningFunctor };
		owningFunctor->table = vTable;
		owningFunctor->memory = std::unique_ptr<char[]>{ new char[size] };
		vTable(VTableModes::MoveConstruct, functor, owningFunctor->memory.get());

		context.mInput.mEntries[indexOfCurrentCategory].mOnDisplayEnd =
			[data = owningFunctor]() -> void
			{
				data->table(VTableModes::Invoke, data->memory.get(), nullptr);
			};
	}

	context.mPushStack.pop();
}

void ImSearch::TextUnformatted(const char* text)
{
	if (ImSearch::PushSearchable(text, [](const char* str) { ImGui::TextUnformatted(str); return true; }))
	{
		ImSearch::PopSearchable();
	}
}

bool ImSearch::TreeNode(const char* text)
{
	return ImSearch::PushSearchable(text, 
		[](const char* str)
		{
			return ImGui::TreeNode(str);
		});
}

void ImSearch::TreePop()
{
	ImSearch::PopSearchable(
		[]()
		{
			ImGui::TreePop();
		});
}

namespace
{
	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput)
	{
		const Input& oldInput = oldResult.mInput;

		if (oldInput.mUserQuery != currentInput.mUserQuery
			|| oldInput.mEntries.size() != currentInput.mEntries.size())
		{
			return false;
		}

		// C++11 didnt have nice algorithms for comparing ranges :(
		for (size_t i = 0; i < oldInput.mEntries.size(); i++)
		{
			if (oldInput.mEntries[i].mText != currentInput.mEntries[i].mText)
			{
				return false;
			}
		}
		return true;
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

		ImSearch::StringMatcher matcher{ input.mUserQuery.c_str() };

		for (IndexT i = 0; i < static_cast<IndexT>(input.mEntries.size()); i++)
		{
			const std::string& text = input.mEntries[i].mText;

			const float score = matcher(text.c_str());
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
		std::stable_sort(buffers.mIndicesToAddToDisplayOrder.begin() + startInIndicesBuffer,
			buffers.mIndicesToAddToDisplayOrder.begin() + endInIndicesBuffer,
			[&](IndexT lhsIndex, IndexT rhsIndex) -> bool
			{
				const float lhsScore = buffers.mScores[lhsIndex];
				const float rhsScore = buffers.mScores[rhsIndex];

				return lhsScore > rhsScore;
			});

		for (IndexT indexInIndicesBuffer = startInIndicesBuffer; indexInIndicesBuffer < endInIndicesBuffer; indexInIndicesBuffer++)
		{
			IndexT searchableIndex = buffers.mIndicesToAddToDisplayOrder[indexInIndicesBuffer];

			const IndexT nextStartInIndicesBuffer = static_cast<IndexT>(buffers.mIndicesToAddToDisplayOrder.size());

			output.mDisplayOrder.emplace_back(searchableIndex);

			const Searchable& searchable = input.mEntries[searchableIndex];
			for (IndexT childIndex = searchable.mIndexOfFirstChild;
				childIndex != sNullIndex;
				childIndex = input.mEntries[childIndex].mIndexOfNextSibling)
			{
				if (buffers.mScores[childIndex] >= sCutOffStrength)
				{
					buffers.mIndicesToAddToDisplayOrder.emplace_back(childIndex);
				}
			}

			const IndexT nextEndInIndicesBuffer = static_cast<IndexT>(buffers.mIndicesToAddToDisplayOrder.size());

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
		buffers.mIndicesToAddToDisplayOrder.clear();

		for (IndexT i = 0; i < static_cast<IndexT>(input.mEntries.size()); i++)
		{
			if (input.mEntries[i].mIndexOfParent == sNullIndex
				&& buffers.mScores[i] >= sCutOffStrength)
			{
				buffers.mIndicesToAddToDisplayOrder.emplace_back(i);
			}
		}

		AppendToDisplayOrder(input, 
			buffers,
			0,
			static_cast<IndexT>(buffers.mIndicesToAddToDisplayOrder.size()),
			output);
	}

	void BringResultUpToDate(Result& result)
	{
		AssignInitialScores(result.mInput, result.mBuffers);
		PropagateScoreToChildren(result.mInput, result.mBuffers);
		PropagateScoreToParents(result.mInput, result.mBuffers);
		GenerateDisplayOrder(result.mInput, result.mBuffers, result.mOutput);
	}

	void DisplayToUser(const Result& result)
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

			if (isEnd)
			{
				if (searchable.mOnDisplayEnd)
				{
					searchable.mOnDisplayEnd();
				}
				continue;
			}

			if (!searchable.mOnDisplayStart)
			{
				continue;
			}

			if (isUserSearching)
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}

			if (searchable.mOnDisplayStart(searchable.mText.c_str()))
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

ImSearch::StringMatcher::StringMatcher(const char* userQuery) :
	mUserQuery(userQuery),
	mUserLen(strlen(userQuery))
{
}

float ImSearch::StringMatcher::operator()(const char* text_body) const
{
	if (mUserLen == 0)
	{
		return 1.0f;
	}

	size_t current = 0;
	for (size_t i = 0; text_body[i] != '\0'; ++i)
	{
		if (text_body[i] == mUserQuery[current])
		{
			current++;
			if (current == mUserLen)
			{
				break;
			}
		}
	}

	return static_cast<float>(current) / static_cast<float>(mUserLen);
}

#endif // #ifndef IMGUI_DISABLE
