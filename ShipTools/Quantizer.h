/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <array>
#include <optional>
#include <string>

class Quantizer
{
public:

    static void Quantize(
        std::string const & inputFile,
        std::string const & outputFile,
        std::string const & materialsFile,
        bool doKeepRopes,
        bool doKeepGlass,
        std::optional<std::array<uint8_t, 3u>> targetFixedColor);
};
