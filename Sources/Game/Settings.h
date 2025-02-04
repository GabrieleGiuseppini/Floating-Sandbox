/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "FileSystem.h"
#include "GameVersion.h"

#include <Core/Streams.h>
#include <Core/Utils.h>

#include <picojson.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <typeinfo>
#include <type_traits>
#include <utility>
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

enum class PersistedSettingsStorageTypes
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
	PersistedSettingsStorageTypes StorageType;

    PersistedSettingsKey(
        std::string const & name,
		PersistedSettingsStorageTypes storageType)
        : Name(name)
        , StorageType(storageType)
    {}

    bool operator==(PersistedSettingsKey const & other) const
    {
        return Name == other.Name
            && StorageType == other.StorageType;
    }

	bool operator!=(PersistedSettingsKey const & other) const
	{
		return !(*this == other);
	}

    static PersistedSettingsKey MakeLastModifiedSettingsKey()
    {
        return PersistedSettingsKey("Last Modified", PersistedSettingsStorageTypes::User);
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

    static PersistedSettingsMetadata MakeLastModifiedSettingsMetadata()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss
            << "The settings that were last modified when the game was played on "
            << std::put_time(std::localtime(&now_c), "%F at %T")
            << ".";

        return PersistedSettingsMetadata(
            PersistedSettingsKey::MakeLastModifiedSettingsKey(),
            ss.str());
    }
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

    bool HasSettings(PersistedSettingsKey const & settingsKey) const;

    std::vector<PersistedSettingsMetadata> ListSettings() const;

    void Delete(PersistedSettingsKey const & settingsKey);

    // Returns nullptr if file does not exist
    std::unique_ptr<BinaryReadStream> OpenBinaryInputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension) const;

    // Returns nullptr if file does not exist
    std::unique_ptr<TextReadStream> OpenTextInputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension) const;

    std::unique_ptr<BinaryWriteStream> OpenBinaryOutputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension);

    std::unique_ptr<TextWriteStream> OpenTextOutputStream(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension);

private:

    void ListSettings(
        std::filesystem::path directoryPath,
		PersistedSettingsStorageTypes storageType,
        std::vector<PersistedSettingsMetadata> & outPersistedSettingsMetadata) const;

    std::filesystem::path MakeFilePath(
        PersistedSettingsKey const & settingsKey,
        std::string const & streamName,
        std::string const & extension) const;

    std::filesystem::path GetRootPath(PersistedSettingsStorageTypes storageType) const;

    std::filesystem::path const mRootSystemSettingsDirectoryPath;
    std::filesystem::path const mRootUserSettingsDirectoryPath;
    std::shared_ptr<IFileSystem> mFileSystem;
};

class SettingsSerializationContext final
{
public:

    SettingsSerializationContext(
        PersistedSettingsMetadata const & settingsMetadata,
        SettingsStorage & storage)
        : SettingsSerializationContext(
            settingsMetadata.Key,
            settingsMetadata.Description,
            storage)
    {}

    SettingsSerializationContext(
        PersistedSettingsKey const & settingsKey,
        std::string const & description,
        SettingsStorage & storage);

    ~SettingsSerializationContext();

    picojson::object & GetSettingsRoot()
    {
        return *mSettingsRoot;
    }

    std::unique_ptr<BinaryWriteStream> GetNamedBinaryOutputStream(
        std::string const & streamName,
        std::string const & extension)
    {
        return mStorage.OpenBinaryOutputStream(mSettingsKey, streamName, extension);
    }

    std::unique_ptr<TextWriteStream> GetNamedTextOutputStream(
        std::string const & streamName,
        std::string const & extension)
    {
        return mStorage.OpenTextOutputStream(mSettingsKey, streamName, extension);
    }

private:

    PersistedSettingsKey const mSettingsKey;
    SettingsStorage & mStorage;

    picojson::object mSettingsJson;
    picojson::object * mSettingsRoot;
};

class SettingsDeserializationContext final
{
public:

    SettingsDeserializationContext(
        PersistedSettingsKey const & settingsKey,
        SettingsStorage const & storage);

    picojson::object const & GetSettingsRoot() const
    {
        return mSettingsRoot;
    }

    Version const & GetSettingsVersion() const
    {
        return mSettingsVersion;
    }

    std::unique_ptr<BinaryReadStream> GetNamedBinaryInputStream(
        std::string const & streamName,
        std::string const & extension) const
    {
        return mStorage.OpenBinaryInputStream(mSettingsKey, streamName, extension);
    }

    std::unique_ptr<TextReadStream> GetNamedTextInputStream(
        std::string const & streamName,
        std::string const & extension) const
    {
        return mStorage.OpenTextInputStream(mSettingsKey, streamName, extension);
    }

private:

    PersistedSettingsKey const mSettingsKey;
    SettingsStorage const & mStorage;

    picojson::object mSettingsRoot;
    Version mSettingsVersion;
};

/*
 * Maintains the logic for de/serializing values.
 */
struct SettingSerializer
{
    template<typename TValue,
        typename std::enable_if_t<
            !std::is_integral<TValue>::value
            && !std::is_same<TValue, bool>::value
            && !std::is_floating_point<TValue>::value
            && !std::is_enum<TValue>::value>* = nullptr>
    static void Serialize(
        SettingsSerializationContext & context,
        std::string const & settingName,
        TValue const & value);

    template<typename TValue,
        typename std::enable_if_t<
        !std::is_integral<TValue>::value
        && !std::is_same<TValue, bool>::value
        && !std::is_floating_point<TValue>::value
        && !std::is_enum<TValue>::value> * = nullptr>
    static bool Deserialize(
        SettingsDeserializationContext const & context,
        std::string const & settingName,
        TValue & value);

    // Specializations for integral and enum types

    template<typename TValue,
        typename std::enable_if_t<
            (std::is_integral<TValue>::value && !std::is_same<bool, TValue>::value)
            || std::is_enum<TValue>::value> * = nullptr>
    static void Serialize(
        SettingsSerializationContext & context,
        std::string const & settingName,
        TValue const & value)
    {
        context.GetSettingsRoot()[settingName] = picojson::value(static_cast<int64_t>(value));
    }

    template<typename TValue,
        typename std::enable_if_t<
            (std::is_integral<TValue>::value && !std::is_same<bool, TValue>::value)
            || std::is_enum<TValue>::value> * = nullptr>
    static bool Deserialize(
        SettingsDeserializationContext const & context,
        std::string const & settingName,
        TValue & value)
    {
        auto jsonValue = Utils::GetOptionalJsonMember<int64_t>(context.GetSettingsRoot(), settingName);
        if (!!jsonValue)
        {
            value = static_cast<TValue>(*jsonValue);
            return true;
        }

        return false;
    }

    // Specializations for bool

    template<typename TValue, typename std::enable_if_t<std::is_same<bool, TValue>::value> * = nullptr>
    static void Serialize(
        SettingsSerializationContext & context,
        std::string const & settingName,
        bool const & value)
    {
        context.GetSettingsRoot()[settingName] = picojson::value(value);
    }

    template<typename TValue, typename std::enable_if_t<std::is_same<bool, TValue>::value> * = nullptr>
    static bool Deserialize(
        SettingsDeserializationContext const & context,
        std::string const & settingName,
        bool & value)
    {
        auto jsonValue = Utils::GetOptionalJsonMember<bool>(context.GetSettingsRoot(), settingName);
        if (!!jsonValue)
        {
            value = *jsonValue;
            return true;
        }

        return false;
    }

    // Specialization for floating-point types

    template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
    static void Serialize(
        SettingsSerializationContext & context,
        std::string const & settingName,
        TValue const & value)
    {
        context.GetSettingsRoot()[settingName] = picojson::value(static_cast<double>(value));
    }

    template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
    static bool Deserialize(
        SettingsDeserializationContext const & context,
        std::string const & settingName,
        TValue & value)
    {
        auto jsonValue = Utils::GetOptionalJsonMember<double>(context.GetSettingsRoot(), settingName);
        if (!!jsonValue)
        {
            value = static_cast<TValue>(*jsonValue);
            return true;
        }

        return false;
    }
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

    virtual ~BaseSetting()
    {}

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

	std::string GetName() const
	{
		return mName;
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
        mValue = std::move(value);

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

    virtual void Serialize(SettingsSerializationContext & context) const override
    {
        SettingSerializer::Serialize<TValue>(context, GetName(), mValue);
    }

    virtual void Deserialize(SettingsDeserializationContext const & context) override
    {
        if (SettingSerializer::Deserialize<TValue>(context, GetName(), mValue))
            this->MarkAsDirty();
    }

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
        assert(other.mSettings.size() == Size);

        mSettings.reserve(Size);

        for (auto const & s : other.mSettings)
        {
            mSettings.emplace_back(s->Clone());
        }
    }

    Settings(Settings && other)
    {
        assert(other.mSettings.size() == Size);

        mSettings = std::move(other.mSettings);
    }

    explicit Settings(std::vector<std::unique_ptr<BaseSetting>> && settings)
        : mSettings(std::move(settings))
    {
        assert(mSettings.size() == Size);
    }

    explicit Settings(std::vector<std::unique_ptr<BaseSetting>> const & settings)
    {
        assert(settings.size() == Size);

        mSettings.reserve(settings.size());

        for (auto const & s : settings)
        {
            mSettings.emplace_back(s->Clone());
        }
    }

    Settings & operator=(Settings const & other)
    {
        assert(other.mSettings.size() == Size);

        mSettings.clear();

        for (auto const & s : other.mSettings)
        {
            mSettings.emplace_back(s->Clone());
        }

        return *this;
    }

    Settings & operator=(Settings && other)
    {
        assert(other.mSettings.size() == Size);

        mSettings = std::move(other.mSettings);

        return *this;
    }

    bool operator==(Settings const & other) const
    {
        assert(other.mSettings.size() == Size);

        for (size_t s = 0; s < Size; ++s)
        {
            if (!mSettings[s]->IsEqual(*other.mSettings[s]))
                return false;
        }

        return true;
    }

	bool operator!=(Settings const & other) const
	{
		return !(*this == other);
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
        using T = std::remove_cv_t<std::remove_reference_t<TValue>>;

        assert(static_cast<size_t>(settingId) < mSettings.size());
        assert(typeid(T) == mSettings[static_cast<size_t>(settingId)]->GetType());

        auto * s = dynamic_cast<Setting<T> *>(mSettings[static_cast<size_t>(settingId)].get());
        s->SetValue(value);
    }

    template<typename TValue>
    void SetValue(TEnum settingId, TValue && value)
    {
        using T = std::remove_cv_t<std::remove_reference_t<TValue>>;

        assert(static_cast<size_t>(settingId) < mSettings.size());
        assert(typeid(T) == mSettings[static_cast<size_t>(settingId)]->GetType());

        auto * s = dynamic_cast<Setting<T> *>(mSettings[static_cast<size_t>(settingId)].get());
        s->SetValue(std::move(value));
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

    inline static size_t const Size = static_cast<size_t>(TEnum::_Last) + 1;

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

    virtual ~BaseSettingEnforcer()
    {}

    virtual void Enforce(BaseSetting const & setting) const = 0;

	virtual void EnforceImmediate(BaseSetting const & setting) const = 0;

    virtual void Pull(BaseSetting & setting) const = 0;
};

template<typename TValue>
class SettingEnforcer final : public BaseSettingEnforcer
{
public:

    using Getter = std::function<TValue ()>;
    using Setter = std::function<void(TValue const &)>;
	using SetterImmediate = std::function<void(TValue const &)>;

    SettingEnforcer(
        Getter getter,
        Setter setter,
		Setter setterImmediate)
        : mGetter(std::move(getter))
        , mSetter(std::move(setter))
		, mSetterImmediate(std::move(setterImmediate))
    {}

    void Enforce(BaseSetting const & setting) const override
    {
        assert(setting.GetType() == typeid(TValue));

        auto const & s = dynamic_cast<Setting<TValue> const &>(setting);
        mSetter(s.GetValue());
    }

	void EnforceImmediate(BaseSetting const & setting) const override
	{
		assert(setting.GetType() == typeid(TValue));

		auto const & s = dynamic_cast<Setting<TValue> const &>(setting);
		mSetterImmediate(s.GetValue());
	}

    void Pull(BaseSetting & setting) const override
    {
        assert(setting.GetType() == typeid(TValue));

        auto & s = dynamic_cast<Setting<TValue> &>(setting);
        s.SetValue(std::move(mGetter()));
    }

private:

    Getter const mGetter;
    Setter const mSetter;
	Setter const mSetterImmediate;
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

    Enforcers & operator=(Enforcers && other)
    {
        assert(other.size() == static_cast<size_t>(TEnum::_Last) + 1);

        mEnforcers = std::move(other.mEnforcers);

        return *this;
    }

    /*
     * Enforces all and only the settings that are marked as dirty.
     */
    void EnforceDirty(Settings<TEnum> const & settings) const
    {
        for (size_t e = 0; e < static_cast<size_t>(TEnum::_Last) + 1; ++e)
        {
            if (settings[static_cast<TEnum>(e)].IsDirty())
            {
                mEnforcers[e]->Enforce(settings[static_cast<TEnum>(e)]);
            }
        }
    }

	/*
	 * Enforces all and only the settings that are marked as dirty,
	 * using the "immediate" setters.
	 */
	void EnforceDirtyImmediate(Settings<TEnum> const & settings) const
	{
		for (size_t e = 0; e < static_cast<size_t>(TEnum::_Last) + 1; ++e)
		{
			if (settings[static_cast<TEnum>(e)].IsDirty())
			{
				mEnforcers[e]->EnforceImmediate(settings[static_cast<TEnum>(e)]);
			}
		}
	}

    /*
     * Pulls all settings and marks all of them as dirty, unconditionally.
     */
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

    Settings<TEnum> MakeSettings() const
    {
        return mTemplateSettings;
    }

    Settings<TEnum> const & GetDefaults() const
    {
        return mDefaultSettings;
    }

    /*
     * Enforces all and only the settings that are marked as dirty.
     */
    void EnforceDirtySettings(Settings<TEnum> const & settings) const
    {
        mEnforcers.EnforceDirty(settings);
    }

	/*
	 * Enforces all and only the settings that are marked as dirty, using the
	 * "immediate" setters.
	 */
	void EnforceDirtySettingsImmediate(Settings<TEnum> const & settings) const
	{
		mEnforcers.EnforceDirtyImmediate(settings);
	}

    /*
     * Pulls all settings and marks all of them as dirty, unconditionally.
     */
    void Pull(Settings<TEnum> & settings) const
    {
        mEnforcers.Pull(settings);
    }

    /*
     * Pulls all settings and marks all of them as dirty, unconditionally.
     */
    Settings<TEnum> Pull() const
    {
        auto settings = mTemplateSettings;
        Pull(settings);
        return settings;
    }

    //
    // Storage
    //

    auto ListPersistedSettings() const
    {
        return mStorage.ListSettings();
    }

    /*
     * Loads the settings with the specified key.
     *
     * On output only loaded settings will be marked as dirty - the others
     * will be clear and left to the values they had before this method call.
     */
	void LoadPersistedSettings(
		PersistedSettingsKey const & key,
		Settings<TEnum> & settings) const
	{
		SettingsDeserializationContext ctx(key, mStorage);
		settings.Deserialize(ctx);
	}

    /*
     * Saves all and only the settings that are marked as dirty.
     */
    void SaveDirtySettings(
        std::string const & name,
        std::string const & description,
        Settings<TEnum> const & settings)
    {
        SettingsSerializationContext ctx(
            PersistedSettingsKey(
                name,
				PersistedSettingsStorageTypes::User),
            description,
            mStorage);

        settings.SerializeDirty(ctx);
    }

    void DeletePersistedSettings(PersistedSettingsKey const & key)
    {
        mStorage.Delete(key);
    }

    /*
     * Checks whether the last-modified settings are available for
	 * being loaded.
     */
    bool HasLastModifiedSettingsPersisted() const
    {
        return mStorage.HasSettings(PersistedSettingsKey::MakeLastModifiedSettingsKey());
    }

	/*
	 * If the last-modified settings exist, enforces default settings with, on top of them,
	 * the last-modified settings, and returns true. Otherwise, returns false.
	 *
	 * Enforcement is done with the "immediate" setters.
	 */
	bool EnforceDefaultsAndLastModifiedSettings()
	{
		if (HasLastModifiedSettingsPersisted())
		{
			// Get default settings
			Settings<TEnum> settings = mDefaultSettings;

			// Load settings on top of defaults
			{
				SettingsDeserializationContext ctx(
					PersistedSettingsKey::MakeLastModifiedSettingsKey(),
					mStorage);

				settings.Deserialize(ctx);
			}

			// Enforce all settings, using the "immediate" setters
			settings.MarkAllAsDirty();
			EnforceDirtySettingsImmediate(settings);

			return true;
		}
		else
		{
			return false;
		}
	}

    /*
     * Saves the currently-enforced settings as the standard "last-modified" settings,
	 * if at least one setting is different than the defaults.
     */
    bool SaveLastModifiedSettings()
    {
        // Take snapshot
        auto settings = mTemplateSettings;
        Pull(settings);

        // Diff with defaults
        settings.SetDirtyWithDiff(mDefaultSettings);

		// Check if there's anything different than the defaults
		if (settings.IsAtLeastOneDirty())
		{
			// Save settings that are different than defaults
			{
				SettingsSerializationContext ctx(
					PersistedSettingsMetadata::MakeLastModifiedSettingsMetadata(),
					mStorage);

				settings.SerializeDirty(ctx);
			}

			return true;
		}
		else
		{
			return false;
		}
    }

protected:

    class BaseSettingsManagerFactory
    {
    public:

        template<typename TValue>
        void AddSetting(
            TEnum settingId,
            std::string && name,
            typename SettingEnforcer<TValue>::Getter && getter,
            typename SettingEnforcer<TValue>::Setter && setter,
			typename SettingEnforcer<TValue>::Setter && setterImmediate)
        {
            assert(mSettings.size() == static_cast<size_t>(settingId));
            assert(mEnforcers.size() == static_cast<size_t>(settingId));
            (void)settingId;

            mSettings.emplace_back(new Setting<TValue>(std::move(name)));
            mEnforcers.emplace_back(new SettingEnforcer<TValue>(std::move(getter), std::move(setter), std::move(setterImmediate)));
        }

    private:

        friend class BaseSettingsManager;

        std::vector<std::unique_ptr<BaseSetting>> mSettings;
        std::vector<std::unique_ptr<BaseSettingEnforcer>> mEnforcers;
    };

    BaseSettingsManager(
        BaseSettingsManagerFactory && factory,
        std::filesystem::path const & rootSystemSettingsDirectoryPath,
        std::filesystem::path const & rootUserSettingsDirectoryPath,
        std::shared_ptr<TFileSystem> fileSystem = std::make_shared<TFileSystem>())
        : mStorage(
            rootSystemSettingsDirectoryPath,
            rootUserSettingsDirectoryPath,
            std::move(fileSystem))
        , mTemplateSettings(std::move(factory.mSettings))
        , mEnforcers(std::move(factory.mEnforcers))
        , mDefaultSettings(mTemplateSettings)
    {
        // Build defaults
        // (assuming this manager is constructed when all getters deliver
        //  default settings)
        mEnforcers.Pull(mDefaultSettings);
        mDefaultSettings.ClearAllDirty();
    }

private:

    // Storage
    SettingsStorage mStorage;

    // Templates
    Settings<TEnum> mTemplateSettings;
    Enforcers<TEnum> mEnforcers;

    // Default settings
    Settings<TEnum> mDefaultSettings;
};
