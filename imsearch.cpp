
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

	constexpr const char* sDefaultLabel = "##SearchBar";
	constexpr const char* sDefaultHint = "Search";

	bool IsResultUpToDate(const Result& oldResult, const Input& currentInput);
	void BringResultUpToDate(Result& result);
	void DisplayToUser(const Result& result);
}

void ImSearch::BeginSearch()
{
	const ImGuiID imId = ImGui::GetID("Search");
	ImGui::PushID(imId);

	sContextStack.emplace(sContexts[imId]);
	SearchContext& context = sContextStack.top();

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);

	std::string& str = context.mInput.mUserQuery;
	ImGui::InputTextWithHint(sDefaultLabel,
		sDefaultHint,
		(char*)str.c_str(),
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
				data->Buf = (char*)str->c_str();
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

bool ImSearch::PushSearchable(const char* name, std::function<bool(const char*)> displayStart)
{
	SearchContext& context = sContextStack.top();

	context.mInput.mEntries.emplace_back();
	Searchable& searchable = context.mInput.mEntries.back();
	searchable.mText = std::string{ name };
	searchable.mOnDisplayStart = std::move(displayStart);

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

void ImSearch::PopSearchable(std::function<void()> displayEnd)
{
	SearchContext& context = sContextStack.top();
	const size_t indexOfCurrentCategory = context.mPushStack.top();
	context.mInput.mEntries[indexOfCurrentCategory].mOnDisplayEnd = std::move(displayEnd);
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

	void AppendToDisplayOrder(const Input& input, Output& output, const IndexT indexOfSearchable)
	{
		IM_ASSERT(indexOfSearchable < input.mEntries.size());

		output.mDisplayOrder.emplace_back(indexOfSearchable);

		const Searchable& searchable = input.mEntries[indexOfSearchable];
		for (IndexT childIndex = searchable.mIndexOfFirstChild; 
			childIndex != sNullIndex; 
			childIndex = input.mEntries[childIndex].mIndexOfNextSibling)
		{
			AppendToDisplayOrder(input, output, childIndex);
		}

		output.mDisplayOrder.emplace_back(indexOfSearchable | Output::sDisplayEndFlag);
	}

	void BringResultUpToDate(Result& result)
	{
		result.mOutput.mDisplayOrder.clear();
		result.mBuffers.mScores.clear();

		const std::vector<Searchable>& entries = result.mInput.mEntries;
		result.mBuffers.mScores.resize(entries.size());

		if (result.mInput.mUserQuery.empty())
		{
			for (IndexT i = 0; i < static_cast<IndexT>(result.mInput.mEntries.size()); i++)
			{
				if (result.mInput.mEntries[i].mIndexOfParent == sNullIndex)
				{
					AppendToDisplayOrder(result.mInput, result.mOutput, i);
				}
			}
			return;
		}

		ImSearch::StringMatcher matcher{ result.mInput.mUserQuery.c_str() };

		for (size_t i = 0; i < entries.size(); i++)
		{
			const std::string& text = entries[i].mText;

			const float score = matcher(text.c_str());
			result.mBuffers.mScores[i] = score;
			result.mOutput.mDisplayOrder.emplace_back(i);
		}

		std::stable_sort(result.mOutput.mDisplayOrder.begin(), result.mOutput.mDisplayOrder.end(),
			[&](size_t lhsIndex, size_t rhsIndex) -> bool
			{
				const float lhsScore = result.mBuffers.mScores[lhsIndex];
				const float rhsScore = result.mBuffers.mScores[rhsIndex];

				return lhsScore > rhsScore;
			});
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
