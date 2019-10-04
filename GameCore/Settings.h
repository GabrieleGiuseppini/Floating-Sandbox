/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <picojson.h> // TODO: remove if including Utils.h

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
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

    SettingsSerializationContext(SettingsPersistenceFileSystem & fileSystem)
        : mFileSystem(fileSystem)
    {}

    // TODO

private:

    SettingsPersistenceFileSystem & mFileSystem;
};

class SettingsDeserializationContext final
{
public:

    SettingsDeserializationContext(SettingsPersistenceFileSystem & fileSystem)
        : mFileSystem(fileSystem)
    {}

    // TODO

private:

    SettingsPersistenceFileSystem & mFileSystem;
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Settings: these classes provide (temporary) storage for settings. This storage
// is not meant to replace the official storage provided by the settings' owners.
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

    virtual void Deserialize(SettingsDeserializationContext const & context) const = 0;

protected:

    BaseSetting()
        : mIsDirty(false)
    {}

private:

    bool mIsDirty;
};

template<typename TValue>
class Setting final : public BaseSetting
{
public:

    Setting()
        : mValue()
    {}

    explicit Setting(TValue const & value)
        : mValue(value)
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
        return std::make_unique<Setting<TValue>>(mValue);
    }

    virtual void Serialize(SettingsSerializationContext & context) const override
    {
        // TODO
    }

    virtual void Deserialize(SettingsDeserializationContext const & context) const override
    {
        // TODO: set self dirty if we load ourselves
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