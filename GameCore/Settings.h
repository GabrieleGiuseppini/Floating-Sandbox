/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "FileSystem.h"
#include "Version.h"

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <typeinfo>
#include <vector>

/*
 * Implementation of settings that may be serialized, de-serialized, and managed.
 */

///////////////////////////////////////////////////////////////////////////////////////
//
// De/Serialization: interface between settings and the de/serialization storage.
//
// For each persisted setting, we have the following files:
// - "<Name>.settings.json", one and only one
// - "<Name>.<StreamName>.<Extension>", zero or more
//
///////////////////////////////////////////////////////////////////////////////////////

enum class StorageTypes
{
    System,
    User
};

/*
 * The identifier of settings that have been/are going to be persisted.
 */
struct PersistedSettingsKey
{
    std::string Name;
    StorageTypes StorageType;

    PersistedSettingsKey(
        std::string const & name,
        StorageTypes storageType)
        : Name(name)
        , StorageType(storageType)
    {}

    bool operator==(PersistedSettingsKey const & other) const
    {
        return Name == other.Name
            && StorageType == other.StorageType;
    }
};

/*
 * Metadata describing persisted settings.
 */
struct PersistedSettingsMetadata
{
    PersistedSettingsKey Key;
    std::string Description;

    PersistedSettingsMetadata(
        PersistedSettingsKey key,
        std::string description)
        : Key(std::move(key))
        , Description(std::move(description))
    {}
};

/*
 * Abstraction over the storage, implementing high-level operations specific
 * to settings. 
 *
 * Bridges between the settings logic and the
 * IFileSystem interface.
 */
class SettingsStorage final
{
public:

    SettingsStorage(
        std::filesystem::path const & rootSystemSettingsDirectoryPath,
        std::filesystem::path const & rootUserSettingsDirectoryPath,
        std::shared_ptr<IFileSystem> fileSystem);

    std::vector<PersistedSettingsMetadata> ListSettings();

    void Delete(PersistedSettingsKey const & settingsKey);

    std::shared_ptr<std::istream> OpenInputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension);

    std::shared_ptr<std::ostream> OpenOutputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension);

private:

    void ListSettings(
        std::filesystem::path directoryPath,
        StorageTypes storageType,
        std::vector<PersistedSettingsMetadata> & outPersistedSettingsMetadata) const;

    std::filesystem::path MakeFilePath(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension) const;

    std::filesystem::path GetRootPath(StorageTypes storageType) const;

    std::filesystem::path const mRootSystemSettingsDirectoryPath;
    std::filesystem::path const mRootUserSettingsDirectoryPath;
    std::shared_ptr<IFileSystem> mFileSystem;
};

class SettingsSerializationContext final
{
public:

    SettingsSerializationContext(
        PersistedSettingsKey const & settingsKey,
        std::shared_ptr<SettingsStorage> storage);

    ~SettingsSerializationContext();

    picojson::object & GetSettingsRoot()
    {
        return *mSettingsRoot;
    }

    std::shared_ptr<std::ostream> GetNamedStream(
        std::string const & streamName,
        std::string const & extension)
    {
        return mStorage->OpenOutputStream(mSettingsKey, streamName, extension);
    }

private:

    PersistedSettingsKey const mSettingsKey;
    std::shared_ptr<SettingsStorage> mStorage;

    picojson::object mSettingsJson;
    picojson::object * mSettingsRoot;
};

class SettingsDeserializationContext final
{
public:

    SettingsDeserializationContext(
        PersistedSettingsKey const & settingsKey,
        std::shared_ptr<SettingsStorage> storage);

    picojson::object const & GetSettingsRoot() const
    {
        return mSettingsRoot;
    }

    Version const & GetSettingsVersion() const
    {
        return mSettingsVersion;
    }

    std::shared_ptr<std::istream> GetNamedStream(
        std::string const & streamName,
        std::string const & extension) const
    {
        return mStorage->OpenInputStream(mSettingsKey, streamName, extension);
    }

private:

    PersistedSettingsKey const mSettingsKey;
    std::shared_ptr<SettingsStorage> mStorage;
    
    picojson::object mSettingsRoot;
    Version mSettingsVersion;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Settings: these classes provide (temporary) storage for settings that are being
// managed. 
// This storage is not meant to replace the official storage provided by the 
// settings' owners.
//
///////////////////////////////////////////////////////////////////////////////////////

class BaseSetting
{
public:

    bool IsDirty() const
    {
        return mIsDirty;
    }

    void MarkAsDirty()
    {
        mIsDirty = true;
    }

    void ClearDirty()
    {
        mIsDirty = false;
    }

    virtual std::type_info const & GetType() const = 0;

    virtual bool IsEqual(BaseSetting const & other) const = 0;

    virtual std::unique_ptr<BaseSetting> Clone() const = 0;

    virtual void Serialize(SettingsSerializationContext & context) const = 0;

    virtual void Deserialize(SettingsDeserializationContext const & context) = 0;

protected:

    explicit BaseSetting(std::string name)
        : mName(std::move(name))
        , mIsDirty(false)
    {}

    std::string GetName() const
    {
        return mName;
    }

private:

    std::string const mName;
    bool mIsDirty;
};

template<typename TValue>
class Setting final : public BaseSetting
{
public:

    explicit Setting(std::string name)
        : BaseSetting(std::move(name))
        , mValue()
    {}

    Setting(
        std::string name,
        TValue const & value)
        : BaseSetting(std::move(name))
        , mValue(value)
    {}

    TValue const & GetValue() const
    {
        return mValue;
    }

    void SetValue(TValue const & value)
    {
        mValue = value;

        MarkAsDirty();
    }

    void SetValue(TValue && value)
    {
        mValue = value;

        MarkAsDirty();
    }

    virtual std::type_info const & GetType() const override
    {
        return typeid(TValue);
    }

    virtual bool IsEqual(BaseSetting const & other) const override
    {
        assert(other.GetType() == typeid(TValue));

        Setting<TValue> const & o = dynamic_cast<Setting<TValue> const &>(other);
        return mValue == o.mValue;
    }

    virtual std::unique_ptr<BaseSetting> Clone() const override
    {
        return std::make_unique<Setting<TValue>>(GetName(), mValue);
    }

    // This is to be specialized for each setting type
    virtual void Serialize(SettingsSerializationContext & context) const override;

    // This is to be specialized for each setting type
    virtual void Deserialize(SettingsDeserializationContext const & context) override;

private:

    TValue mValue;
};

/*
 * This class implements a container of settings thay are managed
 * together.
 *
 * A settings container is aligned with an enum that provides "ID's" 
 * to address individual settings.
 */
template<typename TEnum>
class Settings
{
public:

    Settings(Settings const & other)
    {
        assert(other.mSettings.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mSettings.reserve(static_cast<size_t>(TEnum::_Last) + 1);

        for (auto const & s : other.mSettings)
        {
            mSettings.emplace_back(s->Clone());
        }
    }

    Settings(Settings && other)
    {
        assert(other.mSettings.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mSettings = std::move(other.mSettings);
    }

    explicit Settings(std::vector<std::unique_ptr<BaseSetting>> && settings)
        : mSettings(std::move(settings))
    {
        assert(mSettings.size() == static_cast<size_t>(TEnum::_Last) + 1);
    }

    explicit Settings(std::vector<std::unique_ptr<BaseSetting>> const & settings)
    {
        assert(settings.size() == static_cast<size_t>(TEnum::_Last) + 1);
        
        mSettings.reserve(settings.size());

        for (auto const & s : settings)
        {
            mSettings.emplace_back(s->Clone());
        }
    }

    Settings & operator=(Settings const & other)
    {
        assert(other.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mSettings.clear();

        for (auto const & s : other.mSettings)
        {
            mSettings.emplace_back(s->Clone());
        }
    }

    Settings & operator=(Settings && other)
    {
        assert(other.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mSettings = std::move(other.mSettings);
    }

    BaseSetting const & operator[](TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        return *mSettings[static_cast<size_t>(settingId)];
    }

    BaseSetting & operator[](TEnum settingId)
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        return *mSettings[static_cast<size_t>(settingId)];
    }

    template<typename TValue>
    TValue const & GetValue(TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());
        assert(typeid(TValue) == mSettings[static_cast<size_t>(settingId)]->GetType());

        Setting<TValue> const * s = dynamic_cast<Setting<TValue> const *>(mSettings[static_cast<size_t>(settingId)].get());
        return s->GetValue();
    }

    template<typename TValue>
    void SetValue(TEnum settingId, TValue const & value)
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());
        assert(typeid(TValue) == mSettings[static_cast<size_t>(settingId)]->GetType());

        Setting<TValue> * s = dynamic_cast<Setting<TValue> *>(mSettings[static_cast<size_t>(settingId)].get());
        s->SetValue(value);
    }

    bool IsDirty(TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        return mSettings[static_cast<size_t>(settingId)]->IsDirty();
    }

    void ClearDirty(TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        mSettings[static_cast<size_t>(settingId)]->ClearDirty();
    }

    void MarkAsDirty(TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        mSettings[static_cast<size_t>(settingId)]->MarkAsDirty();
    }

    bool IsAtLeastOneDirty() const
    {
        return std::any_of(
            mSettings.cbegin(),
            mSettings.cend(),
            [](auto const & s)
            {
                return s->IsDirty();
            });
    }

    void ClearAllDirty()
    {
        for (auto & s : mSettings)
            s->ClearDirty();
    }

    void MarkAllAsDirty()
    {
        for (auto & s : mSettings)
            s->MarkAsDirty();
    }    

    /*
     * Marks each setting as dirty or clean depending on whether or not
     * the setting is different than the corresponding setting in the
     * other settings container.
     */
    void SetDirtyWithDiff(Settings<TEnum> const & other)
    {
        assert(mSettings.size() == other.mSettings.size());

        for (size_t s = 0; s < mSettings.size(); ++s)
        {
            if (!mSettings[s]->IsEqual(*other.mSettings[s]))
            {
                mSettings[s]->MarkAsDirty();
            }
            else
            {
                mSettings[s]->ClearDirty();
            }
        }
    }

    /*
     * Serializes all and only the dirty settings.
     */
    void SerializeDirty(SettingsSerializationContext & context) const
    {
        for (auto const & s : mSettings)
        {
            if (s->IsDirty())
            {
                s->Serialize(context);
            }
        }
    }

    /*
     * De-serializes settings.
     *
     * After this call, settings that were de-serialized are marked as dirty, and
     * settings that were not de-serialized are marked as clean.
     */
    void Deserialize(SettingsDeserializationContext const & context)
    {
        ClearAllDirty();

        for (auto const & s : mSettings)
        {
            s->Deserialize(context);
        }
    }

private:

    std::vector<std::unique_ptr<BaseSetting>> mSettings;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Enforcement: these classes provide the bridging between the actual owners of the
// setting values and the BaseSetting hierarchy instances.
//
///////////////////////////////////////////////////////////////////////////////////////

class BaseSettingEnforcer
{
public:

    virtual void Enforce(BaseSetting const & setting) const = 0;

    virtual void Pull(BaseSetting & setting) const = 0;
};

template<typename TValue>
class SettingEnforcer final : public BaseSettingEnforcer
{
public:

    using Getter = std::function<TValue const & ()>;
    using Setter = std::function<void(TValue const &)>;

    SettingEnforcer(
        Getter getter,
        Setter setter)
        : mGetter(std::move(getter))
        , mSetter(std::move(setter))
    {}

    void Enforce(BaseSetting const & setting) const override
    {
        assert(setting.GetType() == typeid(TValue));

        auto const & s = dynamic_cast<Setting<TValue> const &>(setting);
        mSetter(s.GetValue());
    }

    void Pull(BaseSetting & setting) const override
    {
        assert(setting.GetType() == typeid(TValue));

        auto & s = dynamic_cast<Setting<TValue> &>(setting);
        s.SetValue(mGetter());
    }

private:

    Getter const mGetter;
    Setter const mSetter;
};

/*
 * This class implements a container of enforcers.
 */
template<typename TEnum>
class Enforcers
{
public:

    Enforcers(std::vector<std::unique_ptr<BaseSettingEnforcer>> && enforcers)
        : mEnforcers(std::move(enforcers))
    {
        assert(mEnforcers.size() == static_cast<size_t>(TEnum::_Last) + 1);
    }

    Enforcers(std::vector<std::unique_ptr<BaseSettingEnforcer>> const & enforcers)
    {
        assert(enforcers.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mEnforcers.reserve(enforcers.size());

        for (auto const & e : enforcers)
        {
            mEnforcers.emplace_back(e);
        }
    }

    Enforcers & operator=(Enforcers const & other)
    {
        assert(other.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mEnforcers.clear();

        for (auto const & e : other.mEnforcers)
        {
            mEnforcers.emplace_back(e->Clone());
        }
    }

    Enforcers & operator=(Enforcers && other)
    {
        assert(other.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mEnforcers = std::move(other.mEnforcers);
    }

    void Enforce(Settings<TEnum> const & settings) const
    {
        for (size_t e = 0; e < static_cast<size_t>(TEnum::_Last) + 1; ++e)
        {
            mEnforcers[e]->Enforce(settings[static_cast<TEnum>(e)]);
        }
    }

    void Pull(Settings<TEnum> & settings) const
    {
        for (size_t e = 0; e < static_cast<size_t>(TEnum::_Last) + 1; ++e)
        {
            mEnforcers[e]->Pull(settings[static_cast<TEnum>(e)]);
        }
    }

private:

    std::vector<std::unique_ptr<BaseSettingEnforcer>> mEnforcers;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Base class of settings managers.
//
// Specializations provide the following:
// - Building the "master template" settings, as instances of Setting's and Enforcer's
//
///////////////////////////////////////////////////////////////////////////////////////

template<typename TEnum, typename TFileSystem = FileSystem>
class BaseSettingsManager
{
public:

    Settings<TEnum> const & GetDefaults() const
    {
        assert(!!mDefaultSettings);
        return *mDefaultSettings;
    }

    void Enforce(Settings<TEnum> const & settings) const
    {
        assert(!!mEnforcers);
        mEnforcers->Enforce(settings);
    }

    void Pull(Settings<TEnum> & settings) const
    {
        assert(!!mEnforcers);
        mEnforcers->Pull(settings);
    }

    Settings<TEnum> Pull() const
    {
        auto settings = *mTemplateSettings;
        Pull(settings);
        return settings;
    }

    // TODOHERE

protected:

    BaseSettingsManager(
        std::filesystem::path const & rootSystemSettingsDirectoryPath,
        std::filesystem::path const & rootUserSettingsDirectoryPath,
        std::shared_ptr<TFileSystem> fileSystem = std::make_shared<TFileSystem>())
        : mRootSystemSettingsDirectoryPath(rootSystemSettingsDirectoryPath)
        , mRootUserSettingsDirectoryPath(rootUserSettingsDirectoryPath)
        , mFileSystem(std::move(fileSystem))
    {}

    template<typename TValue>
    void AddSetting(
        TEnum settingId,
        std::string && name,
        typename SettingEnforcer<TValue>::Getter && getter,
        typename SettingEnforcer<TValue>::Setter && setter)
    {
        assert(mTmpSettings.size() == static_cast<size_t>(settingId));
        assert(mTmpEnforcers.size() == static_cast<size_t>(settingId));

        mTmpSettings.emplace_back(new Setting<TValue>(std::move(name)));
        mTmpEnforcers.emplace_back(new SettingEnforcer<TValue>(std::move(getter), std::move(setter)));
    }

    void Initialize()
    {
        // Finalize settings and enforcers
        mTemplateSettings = std::make_unique<Settings<TEnum>>(std::move(mTmpSettings));
        mEnforcers = std::make_unique<Enforcers<TEnum>>(std::move(mTmpEnforcers));

        // Build defaults 
        // (assuming Initialize() is invoked when all getters deliver
        //  default settings)
        mDefaultSettings = std::make_unique<Settings<TEnum>>(*mTemplateSettings);
        mEnforcers->Pull(*mDefaultSettings);
        mDefaultSettings->ClearAllDirty();
    }

private:

    // Configuration
    std::filesystem::path const mRootSystemSettingsDirectoryPath;
    std::filesystem::path const mRootUserSettingsDirectoryPath;
    std::shared_ptr<TFileSystem> const mFileSystem;

    // Temporary containers for building up templates
    std::vector<std::unique_ptr<BaseSetting>> mTmpSettings;
    std::vector<std::unique_ptr<BaseSettingEnforcer>> mTmpEnforcers;

    // Templates
    std::unique_ptr<Settings<TEnum>> mTemplateSettings;
    std::unique_ptr<Enforcers<TEnum>> mEnforcers;

    // Default settings
    std::unique_ptr<Settings<TEnum>> mDefaultSettings;
};
