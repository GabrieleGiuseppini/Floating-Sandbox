/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/Version.h>

#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/*
 * This class checks whether there is an updated version of the game.
 */
class UpdateChecker
{
public:

    enum class UpdateCheckOutcomeType
    {
        HasVersion,

        Error
    };

    struct Outcome
    {
        UpdateCheckOutcomeType OutcomeType;

        // Only populated when OutcomeType==HasVersion
        std::optional<Version> LatestVersion;
        std::string HtmlFeatures;

        static Outcome MakeHasVersionOutcome(
            Version && latestVersion,
            std::string && htmlFeatures)
        {
            return Outcome(
                UpdateCheckOutcomeType::HasVersion,
                std::move(latestVersion),
                std::move(htmlFeatures));
        }

        static Outcome MakeErrorOutcome()
        {
            return Outcome(
                UpdateCheckOutcomeType::Error,
                std::nullopt,
                {});
        }

        Outcome(Outcome const & other) = default;
        Outcome(Outcome && other) = default;

        Outcome & operator=(Outcome const & other) = default;
        Outcome & operator=(Outcome && other) = default;

    private:

        Outcome(
            UpdateCheckOutcomeType outcomeType,
            std::optional<Version> && latestVersion,
            std::string && htmlFeatures)
            : OutcomeType(outcomeType)
            , LatestVersion(std::move(latestVersion))
            , HtmlFeatures(std::move(htmlFeatures))
        {}
    };

public:

    /*
     * Once constructed, starts the check right away.
     */
    UpdateChecker();

    ~UpdateChecker();

    /*
     * When ready returns an outcome, otherwise nullopt.
     */
    std::optional<Outcome> const & GetOutcome() const
    {
        static std::optional<Outcome> OptionalOutcome;

        std::lock_guard<std::mutex> lock(mOutcomeMutex);

        if (!!mOutcome)
            OptionalOutcome = *mOutcome;
        else
            OptionalOutcome.reset();

        return OptionalOutcome;
    }

private:

    static Outcome ParseChangeList(std::string const & changeList);

    void WorkerThread();

private:

    std::unique_ptr<Outcome> mOutcome;
    std::mutex mutable mOutcomeMutex;

    std::thread mWorkerThread;
};
