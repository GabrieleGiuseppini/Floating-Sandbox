/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

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
///////////////////////////////////////////////////////////////////////////////////////

/* 
 * Abstraction of file-system primitives to ease unit tests.
 */
struct SettingsPersistenceFileSystem
{
    // TODO
};

class SettingsSerializationContext final
{
public:

    SettingsSerializationContext(
        std::string const & settingsName,
        std::shared_ptr<SettingsPersistenceFileSystem> fileSystem);

    ~SettingsSerializationContext()
    {
        Serialize();
    }

    void Serialize();

    picojson::object & GetSettingsRoot()
    {
        return mSettingsRoot;
    }

    std::ostream & GetNamedStream(std::string const & streamName);

private:

    std::string const mSettingsName;
    std::shared_ptr<SettingsPersistenceFileSystem> mFileSystem;

    picojson::object mSettingsRoot;
    bool mHasBeenSerialized;
};

class SettingsDeserializationContext final
{
public:

    SettingsDeserializationContext(
        std::string const & settingsName,
        std::shared_ptr<SettingsPersistenceFileSystem> fileSystem);

    picojson::object const & GetSettingsRoot() const
    {
        return mSettingsRoot;
    }

    std::istream & GetNamedStream(std::string const & streamName) const;

private:

    std::string const mSettingsName;
    std::shared_ptr<SettingsPersistenceFileSystem> mFileSystem;

    picojson::object mSettingsRoot;
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

    Settings(std::vector<std::unique_ptr<BaseSetting>> && settings)
        : mSettings(std::move(settings))
    {
        assert(mSettings.size() == static_cast<size_t>(TEnum::_Last) + 1);
    }

    Settings(std::vector<std::unique_ptr<BaseSetting>> const & settings)
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
        assert(setting.GetType() == typeid(float));

        auto const & s = dynamic_cast<Setting<TValue> const &>(setting);
        mSetter(s.GetValue());
    }

    void Pull(BaseSetting & setting) const override
    {
        assert(setting.GetType() == typeid(float));

        auto & s = dynamic_cast<Setting<TValue> &>(setting);
        s.SetValue(mGetter());
    }

private:

    Getter const mGetter;
    Setter const mSetter;
};