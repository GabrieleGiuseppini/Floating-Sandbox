/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <algorithm>
#include <vector>

namespace ShipBuilder {

class ModelValidationIssue final
{
public:

    enum class CheckClassType
    {
        EmptyStructuralLayer,
        StructureTooLarge,
        MissingElectricalSubstratum,
        TooManyLights,
        UnpoweredElectricalComponent,
        UnconsumedElectricalSource,
        UnpoweredEngineComponent,
        UnconsumedEngineSource
    };

    enum class SeverityType
    {
        Error,
        Success,
        Warning
    };

    ModelValidationIssue(
        CheckClassType checkClass,
        SeverityType severity)
        : mCheckClass(checkClass)
        , mSeverity(severity)
    {}

    CheckClassType GetCheckClass() const
    {
        return mCheckClass;
    }

    SeverityType GetSeverity() const
    {
        return mSeverity;
    }

private:

    CheckClassType mCheckClass;
    SeverityType mSeverity;
};

class ModelValidationResults final
{
public:

    ModelValidationResults()
        : mIssues()
        , mHasErrors(false)
        , mHasWarnings(false)
    {
    }

    ModelValidationResults(std::vector<ModelValidationIssue> && issues)
        : mIssues(std::move(issues))
    {
        mHasErrors = std::find_if(
            mIssues.cbegin(),
            mIssues.cend(),
            [](ModelValidationIssue const & issue)
            {
                return issue.GetSeverity() == ModelValidationIssue::SeverityType::Error;
            }) != mIssues.cend();

        mHasWarnings = std::find_if(
            mIssues.cbegin(),
            mIssues.cend(),
            [](ModelValidationIssue const & issue)
            {
                return issue.GetSeverity() == ModelValidationIssue::SeverityType::Warning;
            }) != mIssues.cend();
    }

    bool HasErrors() const
    {
        return mHasErrors;
    }

    bool HasWarnings() const
    {
        return mHasWarnings;
    }

    bool HasErrorsOrWarnings() const
    {
        return mHasErrors || mHasWarnings;
    }

    bool IsEmpty() const
    {
        return mIssues.empty();
    }

    auto const & GetIssues() const
    {
        return mIssues;
    }

private:

    std::vector<ModelValidationIssue> mIssues;

    bool mHasErrors;
    bool mHasWarnings;
};
}