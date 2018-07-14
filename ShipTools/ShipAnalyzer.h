/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <string>

class ShipAnalyzer
{
public:

    struct AnalysisInfo
    {
        float TotalMass;

        float MassPerPoint;
        float BuoyantMassPerPoint;

        float BaricentricX;
        float BaricentricY;

        AnalysisInfo()
            : TotalMass(0.0f)
            , MassPerPoint(0.0f)
            , BuoyantMassPerPoint(0.0f)
            , BaricentricX(0.0f)
            , BaricentricY(0.0f)
        {}
    };

    static AnalysisInfo Analyze(
        std::string const & inputFile,
        std::string const & materialsFile);
};
