/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <cassert>
#include <vector>

class IndexRemap final
{
public:

	explicit IndexRemap(size_t elementCount)
		: mOldToNew(elementCount, NoneElementIndex)
	{
		mNewToOld.reserve(elementCount);
	}

	static IndexRemap MakeIdempotent(size_t elementCount)
	{
		IndexRemap remap(elementCount);
		for (size_t i = 0; i < elementCount; ++i)
		{
			remap.mNewToOld.emplace_back(static_cast<ElementIndex>(i));
			remap.mOldToNew[i] = static_cast<ElementIndex>(i);
		}

		return remap;
	}

	auto const & GetOldIndices() const
	{
		return mNewToOld;
	}

	/*
	 * Adds a oldIndex -> <current size> mapping.
	 */
	void AddOld(ElementIndex oldIndex)
	{
		ElementIndex const newIndex = static_cast<ElementIndex>(mNewToOld.size());
		mNewToOld.emplace_back(oldIndex);
		mOldToNew[oldIndex] = newIndex;
	}

	ElementIndex OldToNew(ElementIndex oldIndex) const
	{
		assert(mOldToNew[oldIndex] != NoneElementIndex);
		return mOldToNew[oldIndex];
	}

	ElementIndex NewToOld(ElementIndex newIndex) const
	{
		assert(newIndex < mNewToOld.size());
		return mNewToOld[newIndex];
	}

private:

	std::vector<ElementIndex> mNewToOld;
	std::vector<ElementIndex> mOldToNew;
};