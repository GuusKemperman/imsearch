#ifndef IMGUI_DISABLE

#include "imsearch.h"
#include "imsearch_internal.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace ImSearch
{
	static bool IsResultUpToDate(const ImSearch::Result& oldResult, const ImSearch::Input& currentInput);
	static void BringResultUpToDate(ImSearch::Result& result);

	static void AssignInitialScores(const Input& input, ReusableBuffers& buffers);
	static void PropagateScoreToChildren(const Input& input, ReusableBuffers& buffers);
	static void PropagateScoreToParents(const Input& input, ReusableBuffers& buffers);
	
	static void GenerateDisplayOrder(const Input& input, ReusableBuffers& buffers, Output& output);
	static void AppendToDisplayOrder(const Input& input, ReusableBuffers& buffers, IndexT startInIndicesBuffer, IndexT endInIndicesBuffer, Output& output);
	
	static void FindStringToAppendOnAutoComplete(const Input& input, Output& output);

	static void DisplayToUser(const ImSearch::LocalContext& context, const ImSearch::Result& result);

	static bool DoDrawnGlyphsMatch(
		const ImDrawList& lhsList,
		const ImDrawIdx* lhsStart,
		const ImDrawList& rhsList,
		const ImDrawIdx* rhsStart,
		int length);

	static void HighlightSubstrings(const char* substrStart,
		const char* substrEnd,
		ImDrawList* drawList,
		int startIdxIdx,
		int endIdxIdx);

	static ImSearch::ImSearchContext* sContext{};
}

//-----------------------------------------------------------------------------
// [SECTION] Definitions from imsearch.h
//-----------------------------------------------------------------------------

ImSearchStyle::ImSearchStyle()
{
	Colors[ImSearchCol_TextHighlighted] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Colors[ImSearchCol_TextHighlightedBg] = { 1.0f, 1.0f, 0.0f, 1.0f };
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

bool ImSearch::BeginSearch(ImSearchFlags flags)
{
	const ImGuiID imId = ImGui::GetID("Search");
	ImGui::PushID(static_cast<int>(imId));

	ImSearchContext& context = GetImSearchContext();
	LocalContext& localContext = context.Contexts[imId];
	context.ContextStack.emplace(localContext);

	localContext.mHasSubmitted = false;
	localContext.mInput.mFlags = flags;

	return true;
}

void ImSearch::SearchBar(const char* hint)
{
	LocalContext& context = GetLocalContext();

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);

	std::string& userQuery = context.mInput.mUserQuery;

	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
		userQuery.clear();
	}

	ImGui::InputTextWithHint("##SearchBar",
		hint,
		const_cast<char*>(userQuery.c_str()),
		userQuery.capacity() + 1,
		ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCompletion,
		+[](ImGuiInputTextCallbackData* data) -> int
		{
			LocalContext* capturedContext = static_cast<LocalContext*>(data->UserData);
			std::string& capturedQuery = capturedContext->mInput.mUserQuery;

			if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
			{
				// Resize string callback
				// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
				IM_ASSERT(data->Buf == capturedQuery.c_str());
				capturedQuery.resize(static_cast<size_t>(data->BufTextLen));
				data->Buf = const_cast<char*>(capturedQuery.c_str());
			}
			if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
			{
				const std::string& newLastToken = capturedContext->mResult.mOutput.mPreviewText;

				if (newLastToken.empty())
				{
					return 0;
				}

				const std::vector<std::string> splitQuery = SplitTokens(capturedQuery);
				
				if (splitQuery.empty())
				{
					return 0;
				}

				const StrView oldLastToken = splitQuery.back();
				
				// Find the start of the last token
				IndexT startOfLastToken = static_cast<IndexT>(capturedQuery.size() - oldLastToken.size());
				while (startOfLastToken > 0)
				{
					if (StrView{ capturedQuery.data() + startOfLastToken, oldLastToken.size() } == oldLastToken)
					{
						break;
					}
					--startOfLastToken;
				}

				data->DeleteChars(static_cast<int>(startOfLastToken), static_cast<int>(capturedQuery.size()) - static_cast<int>(startOfLastToken));
				data->InsertChars(static_cast<int>(startOfLastToken), newLastToken.c_str(), newLastToken.c_str() + newLastToken.size());
			}
			return 0;
		},
		&context);

	// Some logic for drawing the search icon
	const ImVec2 hintSize = ImGui::CalcTextSize(hint);
	ImRect availRect{ ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
	availRect.Min.x += hintSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
	availRect.Max.x = std::max(availRect.Min.x, availRect.Max.x);

	const float barHalfHeight = availRect.GetHeight() * .5f;
	const ImVec2 iconCentre = { availRect.Max.x - barHalfHeight, (availRect.Min.y + availRect.Max.y) * 0.5f };

	const float lensRadius = barHalfHeight * .5f;
	const ImVec2 lensCentre = { iconCentre.x - barHalfHeight * .2f,  iconCentre.y - barHalfHeight * .2f };

	const float handleLength = barHalfHeight * .4f;
	static constexpr float halfSqrt2 = 0.707106781187f;

	const ImVec2 handleTopLeft = { lensCentre.x + lensRadius * halfSqrt2, lensCentre.y + lensRadius * halfSqrt2 };
	const ImVec2 handleBottomRight = { handleTopLeft.x + handleLength, handleTopLeft.y + handleLength };

	const ImVec2 iconTopLeft{ lensCentre.x - lensRadius, lensCentre.y - lensRadius };
	const ImRect iconRect{ iconTopLeft, handleBottomRight };

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 disabledTextCol = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

	if (availRect.Contains(iconRect))
	{
		drawList->AddCircle(lensCentre, lensRadius, disabledTextCol);
		drawList->AddLine(handleTopLeft, handleBottomRight, disabledTextCol);
	}

	if (!userQuery.empty()
		&& ImGui::IsItemFocused())
	{
		// If the preview text was not set by us, 
		// it may not 'complete' what has currently
		// been typed. Check if this is the case.
		const std::vector<std::string> splitQuery = SplitTokens(userQuery);

		if (!splitQuery.empty())
		{
			StrView previewToDisplay = GetStringNeededToCompletePartial(splitQuery.back(), context.mResult.mOutput.mPreviewText);

			if (previewToDisplay.size() > 0)
			{
				ImVec2 completePreviewPos = ImGui::GetItemRectMin();
				completePreviewPos.x += ImGui::CalcTextSize(userQuery.c_str(), userQuery.c_str() + userQuery.size()).x;

				completePreviewPos.x += ImGui::GetStyle().FramePadding.x;
				completePreviewPos.y += ImGui::GetStyle().FramePadding.y;

				drawList->AddText(completePreviewPos, disabledTextCol, previewToDisplay.begin(), previewToDisplay.end());
			}
		}
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

	context.mHasSubmitted = true;
	context.mInput.mEntries.clear();
	context.mDisplayCallbacks.clear();
	IM_ASSERT(context.mPushStackLevel == 0 && "There were more calls to PushSearchable than to PopSearchable");
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

	IM_ASSERT(name != nullptr);
	IM_ASSERT(!context.mHasSubmitted && "Tried calling PushSearchable after EndSearch or Submit");

	if (!CanCollectSubmissions())
	{
		// Invoke the callback immediately if the user
		// is not actively searching, for performance
		// and memory reasons.
		const bool pushResult = Callback::InvokeAsPushSearchable(vTable, functor, name);

		// We only expect a call to PopSearchable if the callback returned true.
		// So we only increment the depth if the callback returned true.
		if (pushResult)
		{
			context.mPushStackLevel++;
		}

		return pushResult;
	}

	context.mInput.mEntries.emplace_back();
	Searchable& searchable = context.mInput.mEntries.back();
	searchable.mText = std::string{ name };

	context.mDisplayCallbacks.emplace_back();
	if (functor != nullptr
		&& vTable != nullptr)
	{
		context.mDisplayCallbacks.back().mOnDisplayStart = Callback{ functor, vTable };
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
	context.mPushStackLevel++;
	return true;
}

void ImSearch::PopSearchable()
{
	Internal::PopSearchable(nullptr, nullptr);
}

void ImSearch::Internal::PopSearchable(void* functor, VTable vTable)
{
	LocalContext& context = GetLocalContext();

	context.mPushStackLevel--;
	IM_ASSERT(context.mPushStackLevel >= 0 && "Called PopSearchable too many times!");

	if (!CanCollectSubmissions())
	{
		if (functor != nullptr
			&& vTable != nullptr)
		{
			Callback::InvokeAsPopSearchable(vTable, functor);
		}

		return;
	}

	const IndexT indexOfCurrentCategory = GetCurrentItem(context);

	if (functor != nullptr
		&& vTable != nullptr)
	{
		context.mDisplayCallbacks[indexOfCurrentCategory].mOnDisplayEnd = Callback{ functor, vTable };
	}

	context.mPushStack.pop();
}

void ImSearch::SetRelevancyBonus(float bonus)
{
	if (!CanCollectSubmissions())
	{
		return;
	}

	LocalContext& context = GetLocalContext();
	const IndexT indexOfCurrentCategory = GetCurrentItem(context);

	context.mInput.mBonuses.resize(indexOfCurrentCategory + 1);
	context.mInput.mBonuses[indexOfCurrentCategory] = bonus;
}

void ImSearch::AddSynonym(const char* synonym)
{
	// Ensure there is an active 'parent' item
	// to add the synonym to; no point in adding
	// synonyms at the root level, and if you did,
	// you did something wrong.
	LocalContext& context = GetLocalContext();

	IM_UNUSED(context);
	IM_ASSERT(context.mPushStackLevel > 0 && "AddSynonym must be called between PushSearchable and PopSearchable. See imsearch_demo.cpp");

	if (!CanCollectSubmissions())
	{
		return;
	}

	if (Internal::PushSearchable(synonym, nullptr, nullptr))
	{
		PopSearchable();
	}
}

void ImSearch::SetUserQuery(const char* query)
{
	LocalContext& context = GetLocalContext();
	context.mInput.mUserQuery = std::string{ query };
}

const char* ImSearch::GetUserQuery()
{
	LocalContext& context = GetLocalContext();
	return context.mInput.mUserQuery.c_str();
}


ImSearchStyle& ImSearch::GetStyle() 
{
	ImSearchContext& context = GetImSearchContext();
	return context.Style;
}

ImU32 ImSearch::GetColorU32(ImSearchCol idx, float alpha_mul)
{
	ImVec4 c = GetStyleColorVec4(idx);
	c.w *= ImGui::GetStyle().Alpha * alpha_mul;
	return ImGui::ColorConvertFloat4ToU32(c);
}

const ImVec4& ImSearch::GetStyleColorVec4(ImSearchCol idx)
{
	ImSearchStyle& style = GetStyle();
	return style.Colors[idx];
}

void ImSearch::PushStyleColor(ImSearchCol idx, ImU32 col)
{
	PushStyleColor(idx, ImGui::ColorConvertU32ToFloat4(col));
}

void ImSearch::PushStyleColor(ImSearchCol idx, const ImVec4& col) 
{
	ImSearchContext& context = GetImSearchContext();

	ImGuiColorMod backup;
	backup.Col = static_cast<ImGuiCol>(idx);
	backup.BackupValue = context.Style.Colors[idx];
	context.ColorModifiers.push_back(backup);
	context.Style.Colors[idx] = col;
}

void ImSearch::PopStyleColor(int count) 
{
	ImSearchContext& context = GetImSearchContext();
	IM_ASSERT_USER_ERROR(count <= context.ColorModifiers.size(), "You can't pop more modifiers than have been pushed!");
	while (count > 0)
	{
		ImGuiColorMod& backup = context.ColorModifiers.back();
		context.Style.Colors[backup.Col] = backup.BackupValue;
		context.ColorModifiers.pop_back();
		count--;
	}
}

//-----------------------------------------------------------------------------
// [SECTION] Definitions from static functions
//-----------------------------------------------------------------------------

bool ImSearch::IsResultUpToDate(const Result& oldResult, const Input& currentInput)
{
	return oldResult.mInput == currentInput;
}

void ImSearch::BringResultUpToDate(Result& result)
{
	AssignInitialScores(result.mInput, result.mBuffers);
	PropagateScoreToChildren(result.mInput, result.mBuffers);
	PropagateScoreToParents(result.mInput, result.mBuffers);
	GenerateDisplayOrder(result.mInput, result.mBuffers, result.mOutput);
	FindStringToAppendOnAutoComplete(result.mInput, result.mOutput);
}

void ImSearch::AssignInitialScores(const Input& input, ReusableBuffers& buffers)
{
	buffers.mScores.clear();
	buffers.mScores.resize(input.mEntries.size());

	const StrView query = input.mUserQuery;
	const std::string tokenSortedQuery = MakeTokenisedString(input.mUserQuery);

	for (IndexT i = 0; i < static_cast<IndexT>(input.mEntries.size()); i++)
	{
		const Searchable& entry = input.mEntries[i];

		float score = WeightedRatio(query,
			tokenSortedQuery,
			entry.mText,
			GetMemoizedTokenisedString(entry.mText),
			buffers);

		if (i < static_cast<IndexT>(input.mBonuses.size()))
		{
			score += input.mBonuses[i];
		}

		buffers.mScores[i] = score;
	}
}

void ImSearch::PropagateScoreToChildren(const Input& input, ReusableBuffers& buffers)
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

void ImSearch::PropagateScoreToParents(const Input& input, ReusableBuffers& buffers)
{
	// Children can only be submitted AFTER their parent.
	// When iterating over the entries in reverse, we will
	// always reach a node's child before the node itself.
	for (IndexT childIndex = static_cast<IndexT>(input.mEntries.size()); childIndex-- > 0;)
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

void ImSearch::GenerateDisplayOrder(const Input& input, ReusableBuffers& buffers, Output& output)
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

void ImSearch::AppendToDisplayOrder(const Input& input,
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

void ImSearch::FindStringToAppendOnAutoComplete(const Input& input, Output& output)
{
	output.mPreviewText.clear();

	const std::vector<std::string> tokensInQuery = SplitTokens(input.mUserQuery);

	if (tokensInQuery.empty())
	{
		return;
	}

	const StrView tokenToComplete = tokensInQuery.back();

	const std::vector<IndexT>& displayOrder = output.mDisplayOrder;

	for (auto it = displayOrder.begin(); it != displayOrder.end(); ++it)
	{
		const IndexT indexAndFlag = *it;
		const IndexT index = indexAndFlag & ~Output::sDisplayEndFlag;
		const IndexT isEnd = indexAndFlag & Output::sDisplayEndFlag;

		if (isEnd)
		{
			continue;
		}

		const Searchable& searchable = input.mEntries[index];
		const std::vector<std::string> tokens = SplitTokens(searchable.mText);

		for (const StrView token : tokens)
		{
			if (GetStringNeededToCompletePartial(tokenToComplete, token).mSize != 0)
			{
				output.mPreviewText = { token.data(), token.size() };
				return;
			}
		}
	}
}

void ImSearch::DisplayToUser(const LocalContext& context, const Result& result)
{
	const std::string& userQuery = result.mInput.mUserQuery;
	const bool isUserSearching = !userQuery.empty();
	ImGui::PushID(isUserSearching);

	const std::vector<IndexT>& displayOrder = result.mOutput.mDisplayOrder;

	const bool hasHighlighting = (context.mInput.mFlags & ImSearchFlags_NoTextHighlighting) == 0;
	if (hasHighlighting)
	{
		BeginHighlightZone(userQuery.c_str());
	}

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

	if (hasHighlighting)
	{
		EndHighlightZone();
	}

	ImGui::PopID();
}

bool ImSearch::DoDrawnGlyphsMatch(const ImDrawList& lhsList,
	const ImDrawIdx* lhsStart,
	const ImDrawList& rhsList,
	const ImDrawIdx* rhsStart,
	int length)
{
	for (int i = 0; i < length; i++)
	{
		const ImVec2 expectedCurr = lhsList.VtxBuffer[lhsStart[i]].uv;
		const ImVec2 actualCurr = rhsList.VtxBuffer[rhsStart[i]].uv;

		// use std::not_equal_to instead of != to silence -Wfloat-equal
		// warnings, for those that want to integrate this library
		// with a stricter codebase.
		if (std::not_equal_to<float>{}(expectedCurr.x, actualCurr.x)
			|| std::not_equal_to<float>{}(expectedCurr.y, actualCurr.y))
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// [SECTION] Definitions from imsearch_internal.h
//-----------------------------------------------------------------------------

ImSearch::Callback::Callback(void* originalFunctor, ImSearch::Internal::VTable vTable) :
	mVTable(vTable)
{
	if (mVTable == nullptr)
	{
		return;
	}

	int size;
	vTable(VTableModes::GetSize, &size, nullptr);
	mUserFunctor = ImGui::MemAlloc(static_cast<size_t>(size));
	IM_ASSERT(mUserFunctor != nullptr);
	vTable(VTableModes::MoveConstruct, originalFunctor, mUserFunctor);
}

ImSearch::Callback::Callback(Callback&& other) noexcept :
	mVTable(other.mVTable),
	mUserFunctor(other.mUserFunctor)
{
	other.mVTable = nullptr;
	other.mUserFunctor = nullptr;
}

ImSearch::Callback& ImSearch::Callback::operator=(Callback&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	ClearData();

	mVTable = other.mVTable;
	mUserFunctor = other.mUserFunctor;

	other.mVTable = nullptr;
	other.mUserFunctor = nullptr;

	return *this;
}

ImSearch::Callback::~Callback()
{
	ClearData();
}

bool ImSearch::Callback::operator()(const char* name) const
{
	return InvokeAsPushSearchable(mVTable, mUserFunctor, name);
}

void ImSearch::Callback::operator()() const
{
	InvokeAsPopSearchable(mVTable, mUserFunctor);
}

bool ImSearch::Callback::InvokeAsPushSearchable(ImSearch::Internal::VTable vTable, void* userFunctor, const char* name)
{
	return vTable(VTableModes::Invoke, userFunctor, const_cast<char*>(name));
}

void ImSearch::Callback::InvokeAsPopSearchable(ImSearch::Internal::VTable vTable, void* userFunctor)
{
	vTable(VTableModes::Invoke, userFunctor, nullptr);
}

void ImSearch::Callback::ClearData()
{
	if (mUserFunctor == nullptr)
	{
		return;
	}

	mVTable(VTableModes::Destruct, mUserFunctor, nullptr);

	ImGui::MemFree(mUserFunctor);
	mUserFunctor = nullptr;
}

bool ImSearch::operator==(const StrView& lhs, const StrView& rhs)
{
	return lhs.mSize == rhs.mSize && 
		(
			(lhs.mData == nullptr && rhs.mData == nullptr) 
			|| memcmp(lhs.mData, rhs.mData, lhs.mSize) == 0
		);
}

bool ImSearch::operator==(const Searchable& lhs, const Searchable& rhs)
{
	return lhs.mText == rhs.mText
		&& lhs.mIndexOfFirstChild == rhs.mIndexOfFirstChild
		&& lhs.mIndexOfLastChild == rhs.mIndexOfLastChild
		&& lhs.mIndexOfParent == rhs.mIndexOfParent
		&& lhs.mIndexOfNextSibling == rhs.mIndexOfNextSibling;
}

bool ImSearch::operator==(const Input& lhs, const Input& rhs)
{
	return lhs.mFlags == rhs.mFlags
		&& lhs.mUserQuery == rhs.mUserQuery
		&& lhs.mEntries == rhs.mEntries
		&& lhs.mBonuses == rhs.mBonuses;
}

ImSearch::LocalContext& ImSearch::GetLocalContext()
{
	ImSearch::ImSearchContext& ImSearchContext = GetImSearchContext();
	IM_ASSERT(!ImSearchContext.ContextStack.empty() && "Not currently in between a ImSearch::BeginSearch and ImSearch::EndSearch");
	return ImSearchContext.ContextStack.top();
}

ImSearch::ImSearchContext& ImSearch::GetImSearchContext()
{
	ImSearch::ImSearchContext* context = ImSearch::GetCurrentContext();
	IM_ASSERT(sContext != nullptr && "An ImSearchContext has not been created, see ImSearch::CreateContext");
	return *context;
}

ImSearch::IndexT ImSearch::GetCurrentItem(ImSearch::LocalContext& context)
{
	IM_ASSERT(!context.mPushStack.empty() && "No active object; can only be called after PushSearchable");
	IM_ASSERT(!context.mHasSubmitted && "No active object; Submit (or EndSearch) has already been called");
	return context.mPushStack.top();
}

bool ImSearch::CanCollectSubmissions()
{
	return *ImSearch::GetUserQuery() != '\0';
}

float ImSearch::GetScore(size_t index)
{
	LocalContext& context = GetLocalContext();
	auto& scores = context.mResult.mBuffers.mScores;

	if (index >= scores.size())
	{
		return -1.0f;
	}

	return scores[index];
}

size_t ImSearch::GetDisplayOrderEntry(size_t index)
{
	LocalContext& context = GetLocalContext();
	auto& displayOrder = context.mResult.mOutput.mDisplayOrder;

	if (index >= displayOrder.size())
	{
		return std::numeric_limits<size_t>::max();
	}
	return displayOrder[index];
}

int ImSearch::GetNumItemsFilteredOut()
{
	const LocalContext& context = GetLocalContext();
	const auto& displayOrder = context.mResult.mOutput.mDisplayOrder;

	int numInDisplayOrder{};

	for (IndexT flagAndIndex : displayOrder)
	{
		numInDisplayOrder += (flagAndIndex & Output::sDisplayEndFlag) == 0;
	}
	const int numSubmitted = static_cast<int>(context.mResult.mInput.mEntries.size());
	IM_ASSERT(numInDisplayOrder <= numSubmitted);

	return numSubmitted - numInDisplayOrder;
}

void ImSearch::SetPreviewText(const char* preview)
{
	GetLocalContext().mResult.mOutput.mPreviewText = std::string{ preview };
}

const char* ImSearch::GetPreviewText()
{
	return GetLocalContext().mResult.mOutput.mPreviewText.c_str();
}

void ImSearch::BeginHighlightZone(const char* textToHighlight)
{
	ImGuiStorage* storage = ImGui::GetStateStorage();	

	std::string* str = new std::string(textToHighlight);
	storage->SetVoidPtr(ImGui::GetID("TextToHighlight"), str);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	storage->SetInt(ImGui::GetID("StartIdxIdx"), drawList->IdxBuffer.size());
}

void ImSearch::EndHighlightZone()
{
	ImGuiStorage* storage = ImGui::GetStateStorage();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	std::string* str = static_cast<std::string*>(storage->GetVoidPtr(ImGui::GetID("TextToHighlight")));
	const int prevIdxEnd = storage->GetInt(ImGui::GetID("StartIdxIdx"));

	HighlightSubstrings(str->c_str(),
		str->c_str() + str->size(),
		drawList,
		prevIdxEnd,
		drawList->IdxBuffer.size());

	delete str;
}

void ImSearch::HighlightSubstrings(const char* substrStart, 
	const char* substrEnd, 
	ImDrawList* drawList, 
	int startIdxIdx,
	int endIdxIdx)
{
	const ImU32 textCol = ImSearch::GetColorU32(ImSearchCol_TextHighlighted);
	const ImU32 textBgCol = ImSearch::GetColorU32(ImSearchCol_TextHighlightedBg);

	ImDrawListSharedData* sharedData = ImGui::GetDrawListSharedData();
	ImDrawList queryDrawList{ sharedData };
	queryDrawList.AddDrawCmd();
	queryDrawList.PushTextureID(ImGui::GetFont()->ContainerAtlas->TexID);
	queryDrawList.PushClipRect({ -INFINITY, -INFINITY }, { INFINITY, INFINITY });

	struct DrawnGlyph
	{
		DrawnGlyph(ImDrawList& l, char character)
		{
			mIdxIdxStart = l.IdxBuffer.size();
			l.AddText(
				{},
				0xffffffff,
				&character,
				&character + 1);
			mIdxIdxEnd = l.IdxBuffer.size();
		}
		int mIdxIdxStart{};
		int mIdxIdxEnd{};
	};
	ImVector<DrawnGlyph> glyphs{};

	for (const char* ch = substrStart; ch < substrEnd; ch++)
	{
		const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(*ch)));
		const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(*ch)));

		glyphs.push_back(DrawnGlyph{ queryDrawList, lower });
		glyphs.push_back(DrawnGlyph{ queryDrawList, upper });
	}

	queryDrawList.PopClipRect();
	queryDrawList.PopTextureID();

	for (int matchStart = startIdxIdx; matchStart < endIdxIdx; matchStart++)
	{
		int matchEnd = matchStart;

		for (int chIdx = 0; chIdx < substrEnd - substrStart; chIdx++)
		{
			bool anyGlyphMatching = false;

			for (int i = 0; i < 2; i++)
			{
				const DrawnGlyph& glyph = glyphs[(chIdx * 2) + i];
				const int glyphLength = glyph.mIdxIdxEnd - glyph.mIdxIdxStart;

				const int nextMatchEnd = matchEnd + glyphLength;

				if (nextMatchEnd <= endIdxIdx
					&& DoDrawnGlyphsMatch(
						queryDrawList,
						queryDrawList.IdxBuffer.Data + glyph.mIdxIdxStart,
						*drawList,
						&drawList->IdxBuffer[matchEnd],
						glyphLength))
				{
					matchEnd = nextMatchEnd;
					anyGlyphMatching = true;
					break;
				}
			}

			if (!anyGlyphMatching)
			{
				matchEnd = matchStart;
				break;
			}
		}

		if (matchStart == matchEnd)
		{
			continue;
		}

		ImVec2 min{ INFINITY, INFINITY };
		ImVec2 max{ -INFINITY, -INFINITY };
		
		for (int idxIdx = matchStart; idxIdx < matchEnd; idxIdx++)
		{
			ImDrawVert& vert = drawList->VtxBuffer[drawList->IdxBuffer[idxIdx]];
			vert.col = textCol;

			min.x = std::min(min.x, vert.pos.x);
			min.y = std::min(min.y, vert.pos.y);

			max.x = std::max(max.x, vert.pos.x);
			max.y = std::max(max.y, vert.pos.y);
		}		

		const ImVec2 padding = ImGui::GetStyle().FramePadding;
		min.x -= padding.x * .5f;
		min.y -= padding.y * .5f;
		max.x += padding.x * .5f;
		max.y += padding.y * .5f;

		drawList->AddRectFilled(min, max, textBgCol);

		const int count = matchEnd - matchStart;
		drawList->PrimReserve(count, 0);

		for (int idxIdx = matchStart; idxIdx < matchEnd; idxIdx++)
		{
			drawList->PrimWriteIdx(drawList->IdxBuffer[idxIdx]);
		}
	}
}

std::vector<std::string> ImSearch::SplitTokens(StrView s)
{
	std::vector<std::string> tokens{};
	std::string current{};
	for (char c : s)
	{
		if (std::isalnum(static_cast<unsigned char>(c)))
		{
			current += c;
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
	return tokens;
}

std::string ImSearch::Join(const std::vector<std::string>& tokens)
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

ImSearch::StrView ImSearch::GetStringNeededToCompletePartial(StrView partial, StrView complete)
{
	if (complete.size() <= partial.size())
	{
		return {};
	}

	const StrView tokenSubStr = { complete.mData, partial.size() };

	for (IndexT i = 0; i < partial.size(); i++)
	{
		if (std::tolower(static_cast<unsigned char>(tokenSubStr[i])) != std::tolower(static_cast<unsigned char>(partial[i])))
		{
			return {};
		}
	}

	return { complete.data() + partial.size(), complete.size() - partial.size() };
}

std::string ImSearch::MakeTokenisedString(StrView original)
{
	std::vector<std::string> targetTokens = SplitTokens(original);
	std::sort(targetTokens.begin(), targetTokens.end());
	std::string processedTarget = Join(targetTokens);
	
	for (char& c : processedTarget)
	{
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}

	return processedTarget;
}

ImSearch::StrView ImSearch::GetMemoizedTokenisedString(const std::string& original)
{
	ImSearch::ImSearchContext& context = GetImSearchContext();
	auto& preprocessedStrings = context.mTokenisedStrings;

	auto it = context.mTokenisedStrings.find(original);

	if (it == preprocessedStrings.end())
	{
		it = preprocessedStrings.emplace(original, MakeTokenisedString(original)).first;
	}

	return it->second;
}

// This file has been altered to better fit ImSearch.
// The original can be found here https://github.com/Tmplt/python-Levenshtein/blob/master/Levenshtein.c
ImSearch::IndexT ImSearch::LevenshteinDistance(
	StrView s1,
	StrView s2,
	ReusableBuffers& buffers)
{
	IndexT i;
	IndexT* row;  /* we only need to keep one row of costs */
	IndexT* end;

	/* strip common prefix */
	while (s1.mSize > 0 
		&& s2.mSize > 0 
		&& *s1.mData == *s2.mData) 
	{
		s1.mSize--;
		s2.mSize--;
		s1.mData++;
		s2.mData++;
	}

	/* strip common suffix */
	while (s1.mSize > 0 && s2.mSize > 0 && s1.mData[s1.mSize - 1] == s2.mData[s2.mSize - 1]) 
	{
		s1.mSize--;
		s2.mSize--;
	}

	/* catch trivial cases */
	if (s1.mSize == 0)
	{
		return s2.mSize;
	}
	if (s2.mSize == 0)
	{
		return s1.mSize;
	}

	/* make the inner cycle (i.e. s2.mData) the longer one */
	if (s1.mSize > s2.mSize) 
	{
		IndexT nx = s1.mSize;
		const char* sx = s1.mData;
		s1.mSize = s2.mSize;
		s2.mSize = nx;
		s1.mData = s2.mData;
		s2.mData = sx;
	}
	/* check s1.mSize == 1 separately */
	if (s1.mSize == 1) 
	{
		return s2.mSize + 1 - 2 * (memchr(s2.mData, *s1.mData, s2.mSize) != nullptr);
	}
	s1.mSize++;
	s2.mSize++;

	/* initialize first row */
	buffers.mTempIndices.resize(s2.mSize);
	row = buffers.mTempIndices.data();
	end = row + s2.mSize - 1;
	for (i = 0; i < s2.mSize; i++)
	{
		row[i] = i;
	}

	/* go through the matrix and compute the costs.  yes, this is an extremely
	 * obfuscated version, but also extremely memory-conservative and relatively
	 * fast.  */
	for (i = 1; i < s1.mSize; i++)
	{
		IndexT* p = row + 1;
		const char char1 = s1.mData[i - 1];
		const char* char2p = s2.mData;
		IndexT D = i;
		IndexT x = i;
		while (p <= end) 
		{
			if (char1 == *(char2p++))
			{
				x = --D;
			}
			else
			{
				x++;
			}
			D = *p;
			D++;
			if (x > D)
			{
				x = D;
			}
			*(p++) = x;
		}
	}
	
	i = *end;
	return i;
}

float ImSearch::Ratio(StrView s1,
	StrView s2,
	ReusableBuffers& buffers)
{
	const IndexT distance = LevenshteinDistance(s1,
		s2,
		buffers);
	const float ratio = 1.0f - static_cast<float>(distance) / static_cast<float>(s1.size() + s2.size());
	return ratio;
}

float ImSearch::PartialRatio(StrView shorter,
	StrView longer,
	ReusableBuffers& buffers)
{
	if (shorter.size() == 0
		|| longer.size() == 0)
	{
		return 0.0f;
	}

	if (shorter.size() > longer.size())
	{
		std::swap(shorter, longer);
	}

	float maxRatio = 0.0f;
	for (IndexT i = 0; i <= longer.size() - shorter.size(); i++)
	{
		const float ratio = Ratio(shorter,
			{ &longer[i], shorter.size() }, // Window into longer
			buffers);
		maxRatio = std::max(maxRatio, ratio);

		if (maxRatio >= 1.0f)
		{
			break;
		}
	}
	return maxRatio;
}

float ImSearch::WeightedRatio(StrView s1,
	StrView s1Tokenised,
	StrView s2,
	StrView s2Tokenised,
	ReusableBuffers& buffers)
{
	float score = Ratio(s1, s2, buffers);

	const IndexT shorterSize = std::min(s1.size(), s2.size());
	const IndexT longerSize = std::max(s1.size(), s2.size());

	if (longerSize <= shorterSize + shorterSize / 2)
	{
		score = std::max(score,
			Ratio(s1Tokenised, s2Tokenised, buffers) * 0.95f);
	}
	else
	{
		const float weight = longerSize > shorterSize * 8 ? 0.5f : .8f;

		score = std::max(score,
			PartialRatio(s1, s2, buffers) * weight);

		score = std::max(score,
			PartialRatio(s1Tokenised, s2Tokenised, buffers) * 0.95f * weight);
	}

	return score;
}

#endif // #ifndef IMGUI_DISABLE


