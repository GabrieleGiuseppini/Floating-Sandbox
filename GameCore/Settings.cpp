/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Settings.h"

#include "Utils.h"

///////////////////////////////////////////////////////////////////////////////////////

SettingsSerializationContext::SettingsSerializationContext(
    std::string const & settingsName,
    std::filesystem::path const & rootUserSettingsDirectoryPath,
    std::shared_ptr<IFileSystem> fileSystem)
    : mSettingsName(std::move(settingsName))
    , mRootUserSettingsDirectoryPath(rootUserSettingsDirectoryPath)
    , mFileSystem(std::move(fileSystem))
    , mSettingsRoot()
    , mHasBeenSerialized(false)
{
}

void SettingsSerializationContext::Serialize()
{
    if (mHasBeenSerialized)
        return;

    //
    // 1. Delete all files for this settings name
    //

    // TODO


    // Remember we have been serialized now
    mHasBeenSerialized = true;
}

std::shared_ptr<std::ostream> SettingsSerializationContext::GetNamedStream(std::string const & streamName)
{    
    // TODO: filename
    //return mFileSystem->OpenOutputStream()
    // TODO
    return std::shared_ptr<std::ostream>(new std::stringstream());
}

SettingsDeserializationContext::SettingsDeserializationContext(
    std::string const & settingsName,
    std::filesystem::path const & rootSystemSettingsDirectoryPath,
    std::filesystem::path const & rootUserSettingsDirectoryPath,
    std::shared_ptr<IFileSystem> fileSystem)
    : mSettingsName(std::move(settingsName))
    , mRootSystemSettingsDirectoryPath(rootSystemSettingsDirectoryPath)
    , mRootUserSettingsDirectoryPath(rootUserSettingsDirectoryPath)
    , mFileSystem(std::move(fileSystem))
    , mSettingsRoot()
{
    // TODO: load json
}

std::shared_ptr<std::istream> SettingsDeserializationContext::GetNamedStream(std::string const & streamName) const
{
    // TODO
    return std::shared_ptr<std::istream>(new std::stringstream());
}

///////////////////////////////////////////////////////////////////////////////////////

template<>
void Setting<float>::Serialize(SettingsSerializationContext & context) const
{
    auto settingsRoot = context.GetSettingsRoot();

    settingsRoot[GetName()] = picojson::value(static_cast<double>(mValue));
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
    auto settingsRoot = context.GetSettingsRoot();

    settingsRoot[GetName()] = picojson::value(static_cast<int64_t>(mValue));
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
    auto settingsRoot = context.GetSettingsRoot();

    settingsRoot[GetName()] = picojson::value(mValue);
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
    auto settingsRoot = context.GetSettingsRoot();

    settingsRoot[GetName()] = picojson::value(mValue);
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

