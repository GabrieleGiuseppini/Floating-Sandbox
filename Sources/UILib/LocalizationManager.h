/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-08-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <Core/UserGameException.h>

#include <wx/intl.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class LocalizationManager
{
public:

    struct LanguageInfo
    {
        std::string Name;
        std::string Identifier;
        wxLanguage LanguageId;

        LanguageInfo(
            std::string name,
            std::string identifier,
            wxLanguage languageId)
            : Name(name)
            , Identifier(identifier)
            , LanguageId(languageId)
        {}
    };

public:

    static std::unique_ptr<LocalizationManager> CreateInstance(
        std::optional<std::string> desiredLanguageIdentifier,
        GameAssetManager const & gameAssetManager);

    /*
     * Returns the desired UI language - which is enforced only at startup.
     * An empty language means "default", i.e. OS-driven.
     */
    std::optional<LanguageInfo> const & GetDesiredLanguage() const
    {
        return mDesiredLanguage;
    }

    /*
     * Stores - but doesn't change - the specified language as the new UI language.
     * An empty argument implies "default", i.e. OS-driven.
     */
    void StoreDesiredLanguage(std::optional<std::string> const & languageIdentifier);

    /*
     * Gets the identifier of the language currently enforced.
     * Not guaranteed to be in the "available languages" list.
     */
    std::string const & GetEnforcedLanguageIdentifier() const
    {
        return mEnforcedLanguageIdentifier;
    }

    LanguageInfo const & GetDefaultLanguage() const
    {
        return mDefaultLanguage;
    }

    std::string const & GetDefaultLanguageIdentifier() const
    {
        return mDefaultLanguage.Identifier;
    }

    std::vector<LanguageInfo> const & GetAvailableLanguages() const
    {
        return mAvailableLanguages;
    }

    wxString MakeErrorMessage(UserGameException const & exception) const;

private:

    LocalizationManager(
        std::optional<LanguageInfo> && desiredLanguage,
        std::string && enforcedLanguageIdentifier,
        std::vector<LanguageInfo> && availableLanguages,
        std::unique_ptr<wxLocale> && locale)
        : mDesiredLanguage(std::move(desiredLanguage))
        , mEnforcedLanguageIdentifier(std::move(enforcedLanguageIdentifier))
        , mDefaultLanguage(MakeDefaultLanguage())
        , mAvailableLanguages(std::move(availableLanguages))
        , mLocale(std::move(locale))
    {}

    static std::vector<LanguageInfo> MakeAvailableLanguages(GameAssetManager const & gameAssetManager);

    static LanguageInfo MakeDefaultLanguage();

    static LanguageInfo const * FindLanguageInfo(
        wxLanguage languageId,
        std::vector<LanguageInfo> const & availableLanguages);

    static LanguageInfo const * FindLanguageInfo(
        std::string const & languageIdentifier,
        std::vector<LanguageInfo> const & availableLanguages);

private:

    std::optional<LanguageInfo> mDesiredLanguage; // Also storage of UI preference

    std::string const mEnforcedLanguageIdentifier;
    LanguageInfo const mDefaultLanguage;
    std::vector<LanguageInfo> const mAvailableLanguages;

    std::unique_ptr<wxLocale> mLocale;
};