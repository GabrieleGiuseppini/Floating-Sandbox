/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-08-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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

    static std::unique_ptr<LocalizationManager> CreateInstance(std::optional<std::string> desiredLanguageIdentifier);

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

    LanguageInfo const & GetDefaultLanguage() const
    {
        return mDefaultLanguage;
    }

    std::vector<LanguageInfo> const & GetAvailableLanguages() const
    {
        return mAvailableLanguages;
    }

private:

    LocalizationManager(
        std::optional<LanguageInfo> desiredLanguage,
        std::vector<LanguageInfo> && availableLanguages,
        std::unique_ptr<wxLocale> && locale)
        : mDesiredLanguage(desiredLanguage)
        , mDefaultLanguage(MakeDefaultLanguage())
        , mAvailableLanguages(std::move(availableLanguages))
        , mLocale(std::move(locale))
    {}

    static std::string MakeLanguageIdentifier(wxString const & canonicalLanguageName);

    static std::vector<LanguageInfo> MakeAvailableLanguages();

    static LanguageInfo MakeDefaultLanguage();

    static LanguageInfo const * FindLanguageInfo(
        wxLanguage languageId,
        std::vector<LanguageInfo> const & availableLanguages);

    static LanguageInfo const * FindLanguageInfo(
        std::string const & languageIdentifier,
        std::vector<LanguageInfo> const & availableLanguages);

private:

    std::optional<LanguageInfo> mDesiredLanguage; // Also storage of UI preference

    LanguageInfo const mDefaultLanguage;
    std::vector<LanguageInfo> const mAvailableLanguages;

    std::unique_ptr<wxLocale> mLocale;
};