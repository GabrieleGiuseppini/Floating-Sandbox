#include <GameCore/Settings.h>

#include "gtest/gtest.h"

TEST(SettingsTests, Setting_DefaultConstructor)
{
    Setting<float> fSetting;

    EXPECT_EQ(0.0f, fSetting.GetValue());
    EXPECT_FALSE(fSetting.GetIsDirty());
}

TEST(SettingsTests, Setting_ConstructorValue)
{
    Setting<float> fSetting(5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_FALSE(fSetting.GetIsDirty());
}

TEST(SettingsTests, Setting_SetValue)
{
    Setting<float> fSetting;

    fSetting.SetValue(5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_TRUE(fSetting.GetIsDirty());
}

TEST(SettingsTests, Setting_SetDirty)
{
    Setting<float> fSetting;

    fSetting.SetValue(5.0f);

    EXPECT_TRUE(fSetting.GetIsDirty());

    fSetting.SetIsDirty(false);

    EXPECT_FALSE(fSetting.GetIsDirty());
}

TEST(SettingsTests, Setting_Type)
{
    Setting<float> fSetting;

    EXPECT_EQ(typeid(float), fSetting.GetType());
}

TEST(SettingsTests, Setting_Clone)
{
    Setting<float> fSetting;
    fSetting.SetValue(5.0f);

    std::unique_ptr<BaseSetting> fSettingClone = fSetting.Clone();
    EXPECT_FALSE(fSettingClone->GetIsDirty());
    EXPECT_EQ(typeid(float), fSettingClone->GetType());

    Setting<float> * fSetting2 = dynamic_cast<Setting<float> *>(fSettingClone.get());
    EXPECT_EQ(5.0f, fSetting2->GetValue());
}

/////////////////////////////////////////////////////////

TEST(SettingsTest, Enforcer_Enforce)
{
    Setting<float> fSetting;
    fSetting.SetValue(5.0f);

    float valueBeingSet = 0.0f;

    SettingEnforcer<float> e(
        [&valueBeingSet]() -> float const &
        {
            return valueBeingSet;
        },
        [&valueBeingSet](float const & value)
        {
            valueBeingSet = value;
        });

    e.Enforce(fSetting);

    EXPECT_EQ(valueBeingSet, 5.0f);
}

TEST(SettingsTest, Enforcer_Pull)
{
    Setting<float> fSetting;
    fSetting.SetValue(5.0f);

    float valueBeingSet = 4.0f;

    SettingEnforcer<float> e(
        [&valueBeingSet]() -> float const &
        {
            return valueBeingSet;
        },
        [&valueBeingSet](float const & value)
        {
            valueBeingSet = value;
        });

    fSetting.SetIsDirty(false);

    e.Pull(fSetting);

    EXPECT_EQ(4.0f, fSetting.GetValue());
    EXPECT_TRUE(fSetting.GetIsDirty());
}
