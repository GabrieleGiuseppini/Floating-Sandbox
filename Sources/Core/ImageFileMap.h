/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"

#include <cassert>
#include <optional>
#include <unordered_map>
#include <vector>

/*
 * A map to detect image file duplicates.
 */
template<typename TColor, typename TValue>
class ImageFileMap
{
public:

    ImageFileMap()
    {}

    void Add(
        size_t hash,
        ImageSize const & imageSize,
        TValue const & value)
    {
        std::vector<ValueEntry> & values = mHashMap[hash];
        values.push_back({ imageSize, value });
    }

    std::optional<TValue> Find(
        size_t hash,
        ImageData<TColor> const & image,
        std::function<ImageData<TColor>(TValue const &)> imageLoader) const
    {
        auto const & srchIt = mHashMap.find(hash);
        if (srchIt != mHashMap.cend())
        {
            for (ValueEntry const & valueEntry : srchIt->second)
            {
                if (IsMatch(image, valueEntry, imageLoader))
                {
                    return valueEntry.Value;
                }
            }
        }

        return std::nullopt;
    }

private:

    struct ValueEntry
    {
        ImageSize const Size;
        TValue const Value;
    };

    static bool IsMatch(
        ImageData<TColor> const & image,
        ValueEntry const & valueEntry,
        std::function<ImageData<TColor>(TValue const &)> imageLoader)
    {
        if (image.Size != valueEntry.Size)
        {
            return false;
        }

        auto valueEntryImage = imageLoader(valueEntry.Value);
        assert(valueEntryImage.Size == valueEntry.Size);

        return image == valueEntryImage;
    }

    // Map of entries keyed by hash
    std::unordered_map<size_t, std::vector<ValueEntry>> mHashMap;
};
