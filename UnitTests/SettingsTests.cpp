#include <GameCore/Settings.h>

#include "gtest/gtest.h"

enum TestSettings : size_t
{
    Setting1_float = 0,
    Setting2_uint32,
    Setting3_bool,
    Setting4_string,

    _Last = Setting4_string
};

auto MakeTestSettings()
{
    std::vector<std::unique_ptr<BaseSetting>> settings;
    settings.emplace_back(new Setting<float>());
    settings.emplace_back(new Setting<uint32_t>());
    settings.emplace_back(new Setting<bool>());
    settings.emplace_back(new Setting<std::string>());

    return settings;
}

////////////////////////////////////////////////////////////////

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

TEST(SettingsTests, Settings_SetAndGetValue)
{
    Settings<TestSettings> settings(MakeTestSettings());

    settings.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));

    EXPECT_EQ(242.0f, settings.GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(999u, settings.GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(true, settings.GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("Test!"), settings.GetValue<std::string>(TestSettings::Setting4_string));
}

TEST(SettingsTests, Settings_ManagesDirtyness)
{
    Settings<TestSettings> settings(MakeTestSettings());

    settings.ClearDirty();

    EXPECT_FALSE(settings.GetIsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.GetIsDirty(TestSettings::Setting3_bool));

    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);

    EXPECT_TRUE(settings.GetIsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.GetIsDirty(TestSettings::Setting3_bool));

    settings.MarkAllAsDirty();

    EXPECT_TRUE(settings.GetIsDirty(TestSettings::Setting2_uint32));
    EXPECT_TRUE(settings.GetIsDirty(TestSettings::Setting3_bool));

    settings.ClearDirty();

    EXPECT_FALSE(settings.GetIsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.GetIsDirty(TestSettings::Setting3_bool));
}

/////////////////////////////////////////////////////////

TEST(SettingsTests, Enforcer_Enforce)
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

TEST(SettingsTests, Enforcer_Pull)
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
