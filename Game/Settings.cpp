/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Settings.h"

#include <GameCore/Colors.h>

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

std::vector<PersistedSettingsMetadata> SettingsStorage::ListSettings() const
{
    std::vector<PersistedSettingsMetadata> persistedSettingsMetadata;

    ListSettings(
        mRootSystemSettingsDirectoryPath,
		PersistedSettingsStorageTypes::System,
        persistedSettingsMetadata);

    ListSettings(
        mRootUserSettingsDirectoryPath,
		PersistedSettingsStorageTypes::User,
        persistedSettingsMetadata);

    return persistedSettingsMetadata;
}

bool SettingsStorage::HasSettings(PersistedSettingsKey const & settingsKey) const
{
    auto const settingsFilePath = MakeFilePath(
        settingsKey,
        SettingsStreamName,
        SettingsExtension);

    return mFileSystem->Exists(settingsFilePath);
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
    std::string const & extension) const
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
	PersistedSettingsStorageTypes storageType,
    std::vector<PersistedSettingsMetadata> & outPersistedSettingsMetadata) const
{
    static std::regex const SettingsFilenameRegex("^(.+)\\." + SettingsStreamName + "\\." + SettingsExtension + "$");

    std::smatch filenameMatch;
    for (auto const & filepath : mFileSystem->ListFiles(directoryPath))
    {
        try
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
        catch (std::exception const & exc)
        {
            LogMessage("ERROR: error processing setting file \"", filepath.string(), "\": ", exc.what(),
                ". The file will be ignored.");
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

std::filesystem::path SettingsStorage::GetRootPath(PersistedSettingsStorageTypes storageType) const
{
    switch (storageType)
    {
        case PersistedSettingsStorageTypes::System:
            return mRootSystemSettingsDirectoryPath;

        case PersistedSettingsStorageTypes::User:
            return mRootUserSettingsDirectoryPath;
    }

    assert(false);
    throw std::logic_error("All the enum values have been already addressed");
}

///////////////////////////////////////////////////////////////////////////////////////

SettingsSerializationContext::SettingsSerializationContext(
    PersistedSettingsKey const & settingsKey,
    std::string const & description,
    SettingsStorage & storage)
    : mSettingsKey(settingsKey)
    , mStorage(storage)
    , mSettingsJson()
{
    // Delete all files for this settings name
    mStorage.Delete(mSettingsKey);

    // Prepare json
    mSettingsJson["version"] = picojson::value(Version::CurrentVersion().ToString());
    mSettingsJson["description"] = picojson::value(description);
    mSettingsJson["settings"] = picojson::value(picojson::object());

    mSettingsRoot = &(mSettingsJson["settings"].get<picojson::object>());
}

SettingsSerializationContext::~SettingsSerializationContext()
{
    //
    // Complete serialization: serialize json settings
    //

    std::string const settingsJson = picojson::value(mSettingsJson).serialize(true);

    auto os = mStorage.OpenOutputStream(
        mSettingsKey,
        SettingsStreamName,
        SettingsExtension);

    *os << settingsJson;
}

SettingsDeserializationContext::SettingsDeserializationContext(
    PersistedSettingsKey const & settingsKey,
    SettingsStorage const & storage)
    : mSettingsKey(settingsKey)
    , mStorage(storage)
    , mSettingsRoot()
    , mSettingsVersion(Version::CurrentVersion())
{
    //
    // Load JSON
    //

    auto is = mStorage.OpenInputStream(
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
// Specializations for common types
///////////////////////////////////////////////////////////////////////////////////////

// string

template<>
void SettingSerializer::Serialize<std::string>(
    SettingsSerializationContext & context,
    std::string const & settingName,
    std::string const & value)
{
    context.GetSettingsRoot()[settingName] = picojson::value(value);
}

template<>
bool SettingSerializer::Deserialize<std::string>(
    SettingsDeserializationContext const & context,
    std::string const & settingName,
    std::string & value)
{
    auto jsonValue = Utils::GetOptionalJsonMember<std::string>(context.GetSettingsRoot(), settingName);
    if (!!jsonValue)
    {
        value = *jsonValue;
        return true;
    }

    return false;
}

// rgbColor

template<>
void SettingSerializer::Serialize<rgbColor>(
    SettingsSerializationContext & context,
    std::string const & settingName,
    rgbColor const & value)
{
    context.GetSettingsRoot()[settingName] = picojson::value(value.toString());
}

template<>
bool SettingSerializer::Deserialize<rgbColor>(
    SettingsDeserializationContext const & context,
    std::string const & settingName,
    rgbColor & value)
{
    auto jsonValue = Utils::GetOptionalJsonMember<std::string>(context.GetSettingsRoot(), settingName);
    if (!!jsonValue)
    {
        value = rgbColor::fromString(*jsonValue);
        return true;
    }

    return false;
}

// std::chrono::seconds

template<>
void SettingSerializer::Serialize<std::chrono::seconds>(
	SettingsSerializationContext & context,
	std::string const & settingName,
	std::chrono::seconds const & value)
{
	context.GetSettingsRoot()[settingName] = picojson::value(static_cast<int64_t>(value.count()));
}

template<>
bool SettingSerializer::Deserialize<std::chrono::seconds>(
	SettingsDeserializationContext const & context,
	std::string const & settingName,
	std::chrono::seconds & value)
{
	auto jsonValue = Utils::GetOptionalJsonMember<std::int64_t>(context.GetSettingsRoot(), settingName);
	if (!!jsonValue)
	{
		value = std::chrono::seconds(*jsonValue);
		return true;
	}

	return false;
}

// std::chrono::minutes

template<>
void SettingSerializer::Serialize<std::chrono::minutes>(
	SettingsSerializationContext & context,
	std::string const & settingName,
	std::chrono::minutes const & value)
{
	context.GetSettingsRoot()[settingName] = picojson::value(static_cast<int64_t>(value.count()));
}

template<>
bool SettingSerializer::Deserialize<std::chrono::minutes>(
	SettingsDeserializationContext const & context,
	std::string const & settingName,
	std::chrono::minutes & value)
{
	auto jsonValue = Utils::GetOptionalJsonMember<std::int64_t>(context.GetSettingsRoot(), settingName);
	if (!!jsonValue)
	{
		value = std::chrono::minutes(*jsonValue);
		return true;
	}

	return false;
}