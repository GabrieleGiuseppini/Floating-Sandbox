/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <string>

class Resizer
{
public:

    static void Resize(
        std::string const & inputFile,
        std::string const & outputFile,
        int width);
};
