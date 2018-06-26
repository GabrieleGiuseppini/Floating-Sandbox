/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <string>

class Quantizer
{
public:

    static void Quantize(
        std::string const & inputFile,
        std::string const & outputFile,
        std::string const & materialsFile,
        bool doKeepRopes,
        bool doKeepGlass);
};
