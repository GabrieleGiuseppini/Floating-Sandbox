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
        std::string Name; // From wX
        std::string Identifier; // From wX
        wxLanguage LanguageId; // From wX
        std::string FsLanguageCode; // From our names

        LanguageInfo(
            std::string name,
            std::string identifier,
            wxLanguage languageId,
            std::string fsLanguageCode)
            : Name(name)
            , Identifier(identifier)
            , LanguageId(languageId)
            , FsLanguageCode(fsLanguageCode)
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
     * Gets the identifier of the FS language currently enforced.
     * Guaranteed to be from the "available languages" list.
     */
    std::string const & GetEnforcedFsLanguageCode() const
    {
        return mEnforcedFsLanguageCode;
    }

    LanguageInfo const & GetDefaultLanguage() const
    {
        return mDefaultLanguage;
    }

    std::string const & GetDefaultFsLanguageCode() const
    {
        return mDefaultLanguage.FsLanguageCode;
    }

    std::vector<LanguageInfo> const & GetAvailableLanguages() const
    {
        return mAvailableLanguages;
    }

    wxString MakeErrorMessage(UserGameException const & exception) const;

private:

    LocalizationManager(
        std::optional<LanguageInfo> && desiredLanguage,
        std::string && enforcedFsLanguageCode,
        std::vector<LanguageInfo> && availableLanguages,
        std::unique_ptr<wxLocale> && locale)
        : mDesiredLanguage(std::move(desiredLanguage))
        , mEnforcedFsLanguageCode(std::move(enforcedFsLanguageCode))
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

    std::string const mEnforcedFsLanguageCode;
    LanguageInfo const mDefaultLanguage;
    std::vector<LanguageInfo> const mAvailableLanguages;

    std::unique_ptr<wxLocale> mLocale;
};