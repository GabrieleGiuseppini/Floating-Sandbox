/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UpdateChecker.h"

#include <Game/GameVersion.h>

#include <Core/Log.h>
#include <Core/Utils.h>

#include <SFML/Network/Http.hpp>

#include <cassert>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>

std::string const UpdateHost = "http://floatingsandbox.com";
// Changed in 1.17.0
//std::string const UpdateUrl = "/changes.txt";
std::string const UpdateUrl = "/changes2.txt";

UpdateChecker::UpdateChecker()
    : mOutcome()
    , mOutcomeMutex()
    , mWorkerThread()
{
    // Start thread immediately
    mWorkerThread = std::thread(&UpdateChecker::WorkerThread, this);
}

UpdateChecker::~UpdateChecker()
{
    assert(mWorkerThread.joinable());
    mWorkerThread.join();
}

UpdateChecker::Outcome UpdateChecker::ParseChangeList(std::string const & changeList)
{
    std::string line;

    std::stringstream ss(changeList);

    //
    // Parse version
    //

    std::getline(ss, line);
    Version version = Version::FromString(line);

    //
    // Parse features
    //

    std::string htmlFeatures = Utils::ChangelistToHtml(ss);

    return Outcome::MakeHasVersionOutcome(
        std::move(version),
        std::move(htmlFeatures));
}

void UpdateChecker::WorkerThread()
{
    try
    {
        std::string changesFileContent;

        // FOR TESTING
        ////{
        ////    std::this_thread::sleep_for(std::chrono::seconds(2));
        ////    std::ifstream f("C:\\Users\\Neurodancer\\source\\repos\\Floating-Sandbox\\changes.txt");

        ////    std::unique_ptr<char[]> buf(new char[32768]);
        ////    f.read(buf.get(), 32768);
        ////    changesFileContent = std::string(buf.get(), f.gcount());
        ////}

        {
            sf::Http http;
            http.setHost(UpdateHost);
            sf::Http::Request request(UpdateUrl);
            request.setField("Referer", CurrentGameVersion.ToString());

            // Send the request and check the response
            sf::Http::Response response = http.sendRequest(request, sf::seconds(5.0f));
            sf::Http::Response::Status status = response.getStatus();
            LogMessage("UpdateChecker: StatusCode=" + std::to_string(status));
            if (status != sf::Http::Response::Ok)
            {
                throw std::runtime_error("Status code is " + std::to_string(status));
            }

            changesFileContent = response.getBody();
        }

        {
            std::lock_guard<std::mutex> lock(mOutcomeMutex);

            mOutcome = std::make_unique<Outcome>(ParseChangeList(changesFileContent));

            if (mOutcome->OutcomeType == UpdateCheckOutcomeType::HasVersion)
            {
                LogMessage("UpdateChecker: LatestVersion=" + mOutcome->LatestVersion->ToString());
            }
        }
    }
    catch (...)
    {
        std::lock_guard<std::mutex> lock(mOutcomeMutex);
        mOutcome = std::make_unique<Outcome>(Outcome::MakeErrorOutcome());
    }
}