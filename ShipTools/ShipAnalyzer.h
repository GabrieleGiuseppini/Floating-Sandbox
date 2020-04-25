/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <GameCore/Vectors.h>

#include <string>

class ShipAnalyzer
{
public:

    struct AnalysisInfo
    {
        float TotalMass;

        float AverageMassPerPoint;
        float AverageAirBuoyantMassPerPoint;
        float AverageWaterBuoyantMassPerPoint;

        vec2f CenterOfMass;
        vec2f CenterOfDisplacedDensity; // Assuming fully submerged
        float EquilibriumMomentum; // Assuming fully submerged, in equilibrium

        AnalysisInfo()
            : TotalMass(0.0f)
            , AverageMassPerPoint(0.0f)
            , AverageAirBuoyantMassPerPoint(0.0f)
            , AverageWaterBuoyantMassPerPoint(0.0f)
            , CenterOfMass(vec2f::zero())
            , CenterOfDisplacedDensity(vec2f::zero())
            , EquilibriumMomentum(0.0f)
        {}
    };

    static AnalysisInfo Analyze(
        std::string const & inputFile,
        std::string const & materialsDir);
};
