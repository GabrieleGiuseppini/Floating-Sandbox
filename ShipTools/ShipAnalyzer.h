/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <string>

class ShipAnalyzer
{
public:

    struct WeightInfo
    {
        float TotalMass;
        float MassPerPoint;

        WeightInfo()
            : TotalMass(0.0f)
            , MassPerPoint(0.0f)
        {}
    };

    static WeightInfo Weight(
        std::string const & inputFile,
        std::string const & materialsFile);
};
