/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Settings.h"

#include "Utils.h"

#include <regex>

///////////////////////////////////////////////////////////////////////////////////////

static std::string const SettingsStreamName = "settings";
static std::string const SettingsExtension = "json";

SettingsStorage::SettingsStorage(
    std::filesystem::path const & rootSystemSettingsDirectoryPath,
    std::filesystem::path const & rootUserSettingsDirectoryPath,
    std::shared_ptr<IFileSystem> fileSystem)
    : mRootSystemSettingsDirectoryPath(rootSystemSettingsDirectoryPath)
    , mRootUserSettingsDirectoryPath(rootUserSettingsDirectoryPath)
    , mFileSystem(std::move(fileSystem))
{
    // Create user root directory if it doesn't exist
    mFileSystem->EnsureDirectoryExists(rootUserSettingsDirectoryPath);
}

std::vector<PersistedSettingsMetadata> SettingsStorage::ListSettings()
{
    std::vector<PersistedSettingsMetadata> persistedSettingsMetadata;

    ListSettings(
        mRootSystemSettingsDirectoryPath,
        StorageTypes::System,
        persistedSettingsMetadata);

    ListSettings(
        mRootUserSettingsDirectoryPath,
        StorageTypes::User,
        persistedSettingsMetadata);

    return persistedSettingsMetadata;
}

void SettingsStorage::Delete(PersistedSettingsKey const & settingsKey)
{
    for (auto const & filePath : mFileSystem->ListFiles(GetRootPath(settingsKey.StorageType)))
    {
        std::string const stem = filePath.filename().stem().string();
        if (stem.substr(0, settingsKey.Name.length() + 1) == (settingsKey.Name + "."))
        {
            mFileSystem->DeleteFile(filePath);
        }
    }
}

std::shared_ptr<std::istream> SettingsStorage::OpenInputStream(
    PersistedSettingsKey const & settingsKey,
    std::string const & streamName,
    std::string const & extension)
{
    return mFileSystem->OpenInputStream(
        MakeFilePath(
            settingsKey,
            streamName,
            extension));
}

std::shared_ptr<std::ostream> SettingsStorage::OpenOutputStream(
    PersistedSettingsKey const & settingsKey,
    std::string const & streamName,
    std::string const & extension)
{
    return mFileSystem->OpenOutputStream(
        MakeFilePath(
            settingsKey,
            streamName,
            extension));
}

void SettingsStorage::ListSettings(
    std::filesystem::path directoryPath,
    StorageTypes storageType,
    std::vector<PersistedSettingsMetadata> & outPersistedSettingsMetadata) const
{
    static std::regex const SettingsFilenameRegex("^([^\\.]+)\\." + SettingsStreamName + "\\." + SettingsExtension + "$");

    std::smatch filenameMatch;
    for (auto const & filepath : mFileSystem->ListFiles(directoryPath))
    {
        auto const filename = filepath.filename().string();
        if (std::regex_match(filename, filenameMatch, SettingsFilenameRegex))
        {
            //
            // Good settings
            //

            // Extract name
            assert(filenameMatch.size() == 2);
            std::string settingsName = filenameMatch[1].str();

            // Extract description

            auto is = mFileSystem->OpenInputStream(filepath);
            auto settingsValue = Utils::ParseJSONStream(*is);
            if (!settingsValue.is<picojson::object>())
            {
                throw GameException("JSON settings could not be loaded: root value is not an object");
            }

            auto const & description = Utils::GetMandatoryJsonMember<std::string>(
                settingsValue.get<picojson::object>(),
                "description");

            // Store entry
            outPersistedSettingsMetadata.emplace_back(
                PersistedSettingsKey(settingsName, storageType),
                description);
        }
    }
}

std::filesystem::path SettingsStorage::MakeFilePath(
    PersistedSettingsKey const & settingsKey,
    std::string const & streamName,
    std::string const & extension) const
{
    return GetRootPath(settingsKey.StorageType) / (settingsKey.Name + "." + streamName + "." + extension);
}

std::filesystem::path SettingsStorage::GetRootPath(StorageTypes storageType) const
{
    switch (storageType)
    {
        case StorageTypes::System:
            return mRootSystemSettingsDirectoryPath;

        case StorageTypes::User:
            return mRootUserSettingsDirectoryPath;
    }

    assert(false);
    throw std::logic_error("All the enum values have been already addressed");
}

///////////////////////////////////////////////////////////////////////////////////////

SettingsSerializationContext::SettingsSerializationContext(
    PersistedSettingsKey const & settingsKey,
    std::shared_ptr<SettingsStorage> storage)
    : mSettingsKey(std::move(settingsKey))
    , mStorage(std::move(storage))
    , mSettingsJson()
{
    // Delete all files for this settings name
    mStorage->Delete(mSettingsKey);

    // Prepare json
    mSettingsJson["version"] = picojson::value(Version::CurrentVersion().ToString());
    mSettingsJson["settings"] = picojson::value(picojson::object());
    mSettingsRoot = &(mSettingsJson["settings"].get<picojson::object>());
}

SettingsSerializationContext::~SettingsSerializationContext()
{
    //
    // Complete serialization: serialize json settings
    //

    std::string const settingsJson = picojson::value(mSettingsJson).serialize(true);

    auto os = mStorage->OpenOutputStream(
        mSettingsKey,
        SettingsStreamName,
        SettingsExtension);
    
    *os << settingsJson;
}

SettingsDeserializationContext::SettingsDeserializationContext(
    PersistedSettingsKey const & settingsKey,
    std::shared_ptr<SettingsStorage> storage)
    : mSettingsKey(std::move(settingsKey))
    , mStorage(std::move(storage))
    , mSettingsRoot()
    , mSettingsVersion(Version::CurrentVersion())
{
    //
    // Load JSON
    //

    auto is = mStorage->OpenInputStream(
        mSettingsKey,
        SettingsStreamName,
        SettingsExtension);

    auto settingsValue = Utils::ParseJSONStream(*is);
    if (!settingsValue.is<picojson::object>())
    {
        throw GameException("JSON settings could not be loaded: root value is not an object");
    }

    auto & settingsObject = settingsValue.get<picojson::object>();

    //
    // Extract version
    //

    if (0 == settingsObject.count("version")
        || !settingsObject["version"].is<std::string>())
    {
        throw GameException("JSON settings could not be loaded: missing 'version' attribute");
    }

    mSettingsVersion = Version::FromString(settingsObject["version"].get<std::string>());

    //
    // Extract root
    //

    if (0 == settingsObject.count("settings")
        || !settingsObject["settings"].is<picojson::object>())
    {
        throw GameException("JSON settings could not be loaded: missing 'settings' attribute");
    }

    mSettingsRoot = settingsObject["settings"].get<picojson::object>();
}

///////////////////////////////////////////////////////////////////////////////////////

template<>
void Setting<float>::Serialize(SettingsSerializationContext & context) const
{
    context.GetSettingsRoot()[GetName()] = picojson::value(static_cast<double>(mValue));
}

template<>
void Setting<float>::Deserialize(SettingsDeserializationContext const & context)
{
    auto value = Utils::GetOptionalJsonMember<double>(context.GetSettingsRoot(), GetName());
    if (!!value)
    {
        mValue = static_cast<float>(*value);
        MarkAsDirty();
    }
}

template<>
void Setting<unsigned int>::Serialize(SettingsSerializationContext & context) const
{
    context.GetSettingsRoot()[GetName()] = picojson::value(static_cast<int64_t>(mValue));
}

template<>
void Setting<unsigned int>::Deserialize(SettingsDeserializationContext const & context)
{
    auto value = Utils::GetOptionalJsonMember<int64_t>(context.GetSettingsRoot(), GetName());
    if (!!value)
    {
        mValue = static_cast<unsigned int>(*value);
        MarkAsDirty();
    }
}

template<>
void Setting<bool>::Serialize(SettingsSerializationContext & context) const
{
    context.GetSettingsRoot()[GetName()] = picojson::value(mValue);
}

template<>
void Setting<bool>::Deserialize(SettingsDeserializationContext const & context)
{
    auto value = Utils::GetOptionalJsonMember<bool>(context.GetSettingsRoot(), GetName());
    if (!!value)
    {
        mValue = *value;
        MarkAsDirty();
    }
}

template<>
void Setting<std::string>::Serialize(SettingsSerializationContext & context) const
{
    context.GetSettingsRoot()[GetName()] = picojson::value(mValue);
}

template<>
void Setting<std::string>::Deserialize(SettingsDeserializationContext const & context)
{
    auto value = Utils::GetOptionalJsonMember<std::string>(context.GetSettingsRoot(), GetName());
    if (!!value)
    {
        mValue = *value;
        MarkAsDirty();
    }
}

