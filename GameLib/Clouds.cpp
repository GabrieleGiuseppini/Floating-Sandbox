/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

void Clouds::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Resize clouds vector
    if (gameParameters.NumberOfClouds < mClouds.size())
    {
        mClouds.resize(gameParameters.NumberOfClouds);
    }
    else
    {
        for (size_t c = mClouds.size(); c < gameParameters.NumberOfClouds; ++c)
        {
            mClouds.emplace_back(
                new Cloud(
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 100.0f,    // OffsetX
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.01f,     // SpeedX1
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.04f,     // AmpX
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.01f,     // SpeedX2
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 100.0f,    // OffsetY
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.001f,    // AmpY
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.005f,    // SpeedY
                    0.2f + static_cast<float>(c) / static_cast<float>(c + 3), // OffsetScale - the earlier clouds are smaller
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.05f,     // AmpScale
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.005f));  // SpeedScale
        }
    }

    // Update clouds
    for (auto & cloud : mClouds)
    {
        cloud->Update(
            currentSimulationTime,
            gameParameters.WindSpeed);
    }

}

void Clouds::Render(Render::RenderContext & renderContext) const
{
    renderContext.RenderCloudsStart(mClouds.size());

    for (auto const & cloud : mClouds)
    {
        renderContext.UploadCloud(
            cloud->GetX(),
            cloud->GetY(),
            cloud->GetScale());
    }

    renderContext.RenderCloudsEnd();
}

}