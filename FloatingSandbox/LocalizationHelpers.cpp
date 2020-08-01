/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-08-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LocalizationHelpers.h"

#include <Game/ResourceLocator.h>

#include <GameCore/Log.h>

#include <algorithm>
#include <filesystem>

static wxLanguage constexpr TranslationsMsgIdLangId = wxLANGUAGE_ENGLISH; // The language used for the message id's

static std::string TranslationsDomainName = "ui_strings";

std::vector<LocalizationHelpers::LanguageInfo> LocalizationHelpers::GetAvailableLanguages() const
{
    std::vector<LocalizationHelpers::LanguageInfo> languages;

    //
    // Enumerate all directories under our "languages" root
    //

    for (auto const & entryIt : std::filesystem::directory_iterator(ResourceLocator::GetLanguagesRootPath()))
    {
        if (std::filesystem::is_directory(entryIt.path()))
        {
            std::string const languageName = entryIt.path().stem().string();
            auto const wxLangInfo = wxLocale::FindLanguageInfo(languageName);
            if (wxLangInfo == nullptr)
            {
                LogMessage("WARNING: language directory \"", languageName, "\" is not a recognized language");
            }
            else
            {
                // Make sure there's a .mo file
                auto const moFilePath = entryIt.path() / (TranslationsDomainName + ".mo");
                if (!std::filesystem::exists(moFilePath))
                {
                    LogMessage("WARNING: language directory \"", languageName, "\" does not contain a .mo file");
                }
                else
                {
                    // Accepted as a valid language
                    languages.emplace_back(
                        wxLangInfo->Description.ToStdString(),
                        wxLangInfo->Language);
                }
            }
        }
    }

    //
    // Add the language of our msgid's
    //

    {
        auto const wxEnLangInfo = wxLocale::GetLanguageInfo(TranslationsMsgIdLangId);
        assert(nullptr != wxEnLangInfo);
        languages.emplace_back(
            wxEnLangInfo->Description.ToStdString(),
            wxEnLangInfo->Language);
    }

    //
    // Sort and distinct
    //

    std::sort(
        languages.begin(),
        languages.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Name < rhs.Name;
        });

    languages.erase(
        std::unique(
            languages.begin(),
            languages.end(),
            [](auto const & lhs, auto const & rhs)
            {
                return lhs.Name == rhs.Name;
            }),
        languages.end());

    return languages;
}

void LocalizationHelpers::SetLanguage(std::optional<int> languageIdentifier)
{
    int const langId = languageIdentifier.value_or(TranslationsMsgIdLangId);

    mLocale.reset(new wxLocale);    
    auto res = mLocale->Init(langId);
    if (!res)
    {
        LogMessage("WARNING: locale initialization with language ", langId, " failed");
    }
    else
    {
        // Add catalog for this language
        mLocale->AddCatalogLookupPathPrefix(ResourceLocator::GetLanguagesRootPath().string());
        res = mLocale->AddCatalog(TranslationsDomainName, TranslationsMsgIdLangId);
        if (!res && langId != TranslationsMsgIdLangId) // AddCatalog returns false for msgIdLang
        {
            LogMessage("WARNING: locale catalog initialization with language ", langId, " failed");
        }
        else
        {
            LogMessage("Successfully set language ", langId);
        }
    }
}