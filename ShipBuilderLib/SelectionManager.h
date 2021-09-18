/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"
#include "View.h"

namespace ShipBuilder {

/*
 * Manages selection of regions in various layers.
 *
 * - Maintains current selection
 * - Not really a tool but a pseudo-tool
 * - Takes View so that it can instruct it to render marching ants
 * - Is not responsible for drawing of temporary selection while the user is making it, that is the Selection tool
 * - Read by ClipboardManager pseudo-tool to make clip
 */
// TODO: templated on layer type?
class SelectionManager
{
public:

    SelectionManager(View & view)
        : mView(view)
    {}

protected:

    View & mView;
};

}