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

/*
 * Implementation of settings that may be serialized and de-serialized.
 */

 ///////////////////////////////////////////////////////////////////////////////////////
 // Settings
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

///////////////////////////////////////////////////////////////////////////////////////
// Enforcement
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