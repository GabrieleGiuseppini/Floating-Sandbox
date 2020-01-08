/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-01-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

// TODOTEST
#include <iostream>

class LayoutHelper
{
public:

    template<typename TElement>
    struct LayoutElement
    {
        TElement Element;
        std::optional<std::pair<int, int>> Coordinates;

        LayoutElement(
            TElement const & element,
            std::optional<std::pair<int, int>> const & coordinates)
            : Element(element)
            , Coordinates(coordinates)
        {}
    };

    /*
     * Lays out elements in a grid. Accepts elements with or without fixed grid coordinates.
     * Expected coordinates:
     *  x = 0 is center, x = -1, -2, ... are on the left, x = +1, +2, ... are on the right
     *  y = 0 is top, y = +1, +2, ... are below
     */
    template<typename TElement>
    static void Layout(
        std::vector<LayoutElement<TElement>> layoutElements,
        int maxElementsPerRow,
        std::function<void(int width, int height)> const & onBegin,
        std::function<void(std::optional<TElement> element, int x, int y)> const & onPosition)
    {
        assert(maxElementsPerRow > 0);

        int allElementsCount = static_cast<int>(layoutElements.size());

        //
        // - Split elements;
        // - Calculate max x and y among decorated elements
        //

        std::vector<LayoutElement<TElement>> decoratedElements;
        std::vector<TElement> undecoratedElements;

        int maxDecoratedX = 0;
        int maxDecoratedY = 0;

        for (auto const & element : layoutElements)
        {
            if (!!(element.Coordinates))
            {
                maxDecoratedX = std::max(maxDecoratedX, abs(element.Coordinates->first));
                maxDecoratedY = std::max(maxDecoratedY, element.Coordinates->second);
                decoratedElements.emplace_back(element);
            }
            else
            {
                undecoratedElements.emplace_back(element.Element);
            }
        }

        //
        // Calculate bounding box
        //

        // Bounding box among decorated elements
        int const decoratedWidth = decoratedElements.empty() ? 0 : maxDecoratedX * 2 + 1;
        int const decoratedHeight = decoratedElements.empty() ? 0 : maxDecoratedY + 1;

        // Calculate how many extra cells we need
        int const extraCells = std::max(0, allElementsCount - decoratedWidth * decoratedHeight);

        // Calculate number of extra rows and columns needed for the extra cells
        // - We arbitrarily spread these out over (right) segments that are maxElementsPerRow wide,
        //   even if that means that rows will be more than maxElementsPerRow wide
        int const maxElementsPerRowAvailable = maxElementsPerRow;
        int const extraWidth = extraCells >= maxElementsPerRowAvailable ? maxElementsPerRowAvailable : extraCells;
        int const extraHeight = (extraCells / maxElementsPerRowAvailable) + ((extraCells % maxElementsPerRowAvailable) != 0 ? 1 : 0);

        LogMessage("Layout: decoratedW=", decoratedWidth, ", decoratedH=", decoratedHeight, ", extraW=", extraWidth, ", extraH=", extraHeight);

        // Calculate final bounding box
        int const width = decoratedWidth + extraWidth;
        int const height = decoratedHeight + extraHeight;

        //
        // Announce bounding box
        //

        onBegin(width, height);

        //
        // Sort decorated elements by y, x
        //

        std::sort(
            decoratedElements.begin(),
            decoratedElements.end(),
            [](auto const & lhs, auto const & rhs)
            {
                assert(!!(lhs.Coordinates));
                return lhs.Coordinates->second < rhs.Coordinates->second
                    || (lhs.Coordinates->second == rhs.Coordinates->second && lhs.Coordinates->first < rhs.Coordinates->first);
            });

        //
        // Position all items
        //

        auto decoratedIt = decoratedElements.cbegin();
        auto undecoratedIt = undecoratedElements.cbegin();

        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                std::optional<TElement> positionElement;
                if (decoratedIt != decoratedElements.cend())
                {
                    // Position a decorated element
                    positionElement = decoratedIt->Element;
                    ++decoratedIt;
                }
                else if (undecoratedIt != undecoratedElements.cend())
                {
                    // Position an undecorated element
                    positionElement = *undecoratedIt;
                    ++undecoratedIt;
                }
                else
                {
                    // Position a spacer
                    positionElement = std::nullopt;
                }

                // TODOTEST
                //std::cout << "X=" << (w - width / 2) << " Y=" << h << std::endl;

                onPosition(
                    positionElement,
                    w - width / 2,
                    h);
            }
        }

        assert(decoratedIt == decoratedElements.cend());
        assert(undecoratedIt == undecoratedElements.cend());

    }
};