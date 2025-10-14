/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-11-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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
        TooManyVisibleElectricalPanelElements,
        UnpoweredElectricalComponent,
        UnconsumedElectricalSource,
        UnpoweredEngineComponent,
        UnconsumedEngineSource,
        ExteriorLayerTextureTooLarge,
        InteriorLayerTextureTooLarge
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

    template<typename ... TArgs>
    void AddIssue(TArgs&& ... args)
    {
        ModelValidationIssue issue(std::forward<TArgs>(args)...);

        mHasErrors |= (issue.GetSeverity() == ModelValidationIssue::SeverityType::Error);
        mHasWarnings |= (issue.GetSeverity() == ModelValidationIssue::SeverityType::Warning);

        mIssues.emplace_back(std::move(issue));
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