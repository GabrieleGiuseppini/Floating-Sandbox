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
    std::shared_ptr<SettingsPersistenceFileSystem> fileSystem)
    : mSettingsName(std::move(settingsName))
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

std::ostream & SettingsSerializationContext::GetNamedStream(std::string const & streamName)
{    
    // TODO
    static std::ostringstream oss;
    return oss;
}

SettingsDeserializationContext::SettingsDeserializationContext(
    std::string const & settingsName,
    std::shared_ptr<SettingsPersistenceFileSystem> fileSystem)
    : mSettingsName(std::move(settingsName))
    , mFileSystem(std::move(fileSystem))
    , mSettingsRoot()
{
    // TODO: load json
}

std::istream & SettingsDeserializationContext::GetNamedStream(std::string const & streamName) const
{
    // TODO
    static std::istringstream iss;
    return iss;
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

