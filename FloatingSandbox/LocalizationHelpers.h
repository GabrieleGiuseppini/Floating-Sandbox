/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-08-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/xlocale.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class LocalizationHelpers
{
public:

    struct LanguageInfo
    {
        std::string Name;
        int Identifier;

        LanguageInfo(
            std::string name,
            int identifier)
            : Name(name)
            , Identifier(identifier)
        {}
    };

public:

    static LocalizationHelpers & GetInstance()
    {
        static LocalizationHelpers * instance = new LocalizationHelpers();

        return *instance;
    }

    std::vector<LanguageInfo> GetAvailableLanguages() const;

    void SetLanguage(std::optional<int> languageIdentifier);

private:

    std::unique_ptr<wxLocale> mLocale;
};