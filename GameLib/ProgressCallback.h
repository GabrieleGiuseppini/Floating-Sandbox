/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <functional>
#include <string>

// The progress value is the progress that will be reached at the end of the operation
using ProgressCallback = std::function<void(float progress, std::string const & message)>;
