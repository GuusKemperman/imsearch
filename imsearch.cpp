
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

namespace
{
	struct Searchable
	{
		std::string mText{};
		std::function<bool(const char*)> mOnDisplayStart{};
		std::function<void()> mOnDisplayEnd{};
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
		std::vector<size_t> mDisplayOrder{};
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

		std::stack<size_t> mCategoryStack{};
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
	IM_ASSERT(context.mCategoryStack.empty() && "There were more calls to PushSearchable than to PopSearchable");

	sContextStack.pop();
}

bool ImSearch::PushSearchable(const char* name, std::function<bool(const char*)> displayStart)
{
	SearchContext& context = sContextStack.top();

	context.mInput.mEntries.emplace_back();
	Searchable& searchable = context.mInput.mEntries.back();
	searchable.mText = std::string{ name };
	searchable.mOnDisplayStart = std::move(displayStart);

	context.mCategoryStack.emplace(context.mInput.mEntries.size() - static_cast<size_t>(1ull));
	return true;
}

void ImSearch::PopSearchable(std::function<void()> displayEnd)
{
	SearchContext& context = sContextStack.top();
	const size_t indexOfCurrentCategory = context.mCategoryStack.top();
	context.mInput.mEntries[indexOfCurrentCategory].mOnDisplayEnd = std::move(displayEnd);
	context.mCategoryStack.pop();
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
	return ImSearch::PushSearchable(text, [](const char* str) { return ImGui::TreeNode(str); });
}

void ImSearch::TreePop()
{
	ImSearch::PopSearchable([]{ ImGui::TreePop(); });
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

	void BringResultUpToDate(Result& result)
	{
		result.mOutput.mDisplayOrder.clear();
		result.mBuffers.mScores.clear();

		const std::vector<Searchable>& entries = result.mInput.mEntries;
		result.mBuffers.mScores.resize(entries.size());

		if (result.mInput.mUserQuery.empty())
		{
			result.mOutput.mDisplayOrder.resize(entries.size());
			std::iota(result.mOutput.mDisplayOrder.begin(), result.mOutput.mDisplayOrder.end(), 0);
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

		const std::vector<size_t>& displayOrder = result.mOutput.mDisplayOrder;

		for (const size_t index : displayOrder)
		{
			const Searchable& searchable = result.mInput.mEntries[index];

			if (!searchable.mOnDisplayStart)
			{
				continue;
			}

			if (isUserSearching)
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}

			if (searchable.mOnDisplayStart(searchable.mText.c_str())
				&& searchable.mOnDisplayEnd)
			{
				searchable.mOnDisplayEnd();
			}
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
