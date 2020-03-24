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
        float AirBuoyantMassPerPoint;
        float WaterBuoyantMassPerPoint;

        float BaricentricX;
        float BaricentricY;
        float WaterBuoyantBaricentricX;
        float WaterBuoyantBaricentricY;

        AnalysisInfo()
            : TotalMass(0.0f)
            , MassPerPoint(0.0f)
            , AirBuoyantMassPerPoint(0.0f)
            , WaterBuoyantMassPerPoint(0.0f)
            , BaricentricX(0.0f)
            , BaricentricY(0.0f)
            , WaterBuoyantBaricentricX(0.0f)
            , WaterBuoyantBaricentricY(0.0f)
        {}
    };

    static AnalysisInfo Analyze(
        std::string const & inputFile,
        std::string const & materialsDir);
};
