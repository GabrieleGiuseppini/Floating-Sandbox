/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-10-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

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
// Settings: these classes provide (temporary) storage for settings. This storage
// is not meant to replace the official storage provided by the settings' owners.
//
///////////////////////////////////////////////////////////////////////////////////////
class BaseSetting
{
public:

    bool GetIsDirty() const
    {
        return mIsDirty;
    }

    void SetIsDirty(bool value)
    {
        mIsDirty = value;
    }

    virtual std::type_info const & GetType() const = 0;

    virtual std::unique_ptr<BaseSetting> Clone() const = 0;

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

        SetIsDirty(true);
    }

    void SetValue(TValue && value)
    {
        mValue = value;

        SetIsDirty(true);
    }

    virtual std::type_info const & GetType() const override
    {
        return typeid(TValue);
    }

    virtual std::unique_ptr<BaseSetting> Clone() const override
    {
        return std::make_unique<Setting<TValue>>(mValue);
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

    bool GetIsDirty(TEnum settingId) const
    {
        assert(static_cast<size_t>(settingId) < mSettings.size());

        return mSettings[static_cast<size_t>(settingId)]->GetIsDirty();
    }

    void ClearDirty()
    {
        for (auto & s : mSettings)
            s->SetIsDirty(false);
    }

    void MarkAllAsDirty()
    {
        for (auto & s : mSettings)
            s->SetIsDirty(true);
    }

    // TODOHERE

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