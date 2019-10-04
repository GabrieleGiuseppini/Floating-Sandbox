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
    EXPECT_FALSE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_ConstructorValue)
{
    Setting<float> fSetting(5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_FALSE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_SetValue)
{
    Setting<float> fSetting;

    fSetting.SetValue(5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_TRUE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_MarkAsDirty)
{
    Setting<float> fSetting;

    fSetting.ClearDirty();

    EXPECT_FALSE(fSetting.IsDirty());

    fSetting.MarkAsDirty();

    EXPECT_TRUE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_ClearDirty)
{
    Setting<float> fSetting;

    fSetting.MarkAsDirty();

    EXPECT_TRUE(fSetting.IsDirty());

    fSetting.ClearDirty();

    EXPECT_FALSE(fSetting.IsDirty());
}
TEST(SettingsTests, Setting_Type)
{
    Setting<float> fSetting;

    EXPECT_EQ(typeid(float), fSetting.GetType());
}

TEST(SettingsTests, Setting_IsEqual)
{
    Setting<float> fSetting1;
    fSetting1.SetValue(5.0f);

    Setting<float> fSetting2;
    fSetting2.SetValue(15.0f);

    Setting<float> fSetting3;
    fSetting3.SetValue(5.0f);

    EXPECT_FALSE(fSetting1.IsEqual(fSetting2));
    EXPECT_TRUE(fSetting1.IsEqual(fSetting3));
}

TEST(SettingsTests, Setting_Clone)
{
    Setting<float> fSetting;
    fSetting.SetValue(5.0f);

    std::unique_ptr<BaseSetting> fSettingClone = fSetting.Clone();
    EXPECT_FALSE(fSettingClone->IsDirty());
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

TEST(SettingsTests, Settings_IsAtLeastOneDirty)
{
    Settings<TestSettings> settings(MakeTestSettings());

    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));

    EXPECT_TRUE(settings.IsAtLeastOneDirty());

    settings.ClearDirty(TestSettings::Setting2_uint32);

    EXPECT_TRUE(settings.IsAtLeastOneDirty());

    settings.ClearDirty(TestSettings::Setting4_string);

    EXPECT_FALSE(settings.IsAtLeastOneDirty());
}

TEST(SettingsTests, Settings_AllDirtyness)
{
    Settings<TestSettings> settings(MakeTestSettings());

    settings.ClearAllDirty();

    EXPECT_FALSE(settings.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.IsDirty(TestSettings::Setting3_bool));

    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);

    EXPECT_TRUE(settings.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.IsDirty(TestSettings::Setting3_bool));

    settings.MarkAllAsDirty();

    EXPECT_TRUE(settings.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_TRUE(settings.IsDirty(TestSettings::Setting3_bool));

    settings.ClearAllDirty();

    EXPECT_FALSE(settings.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings.IsDirty(TestSettings::Setting3_bool));
}

TEST(SettingsTests, Settings_SetDirtyWithDiff)
{
    Settings<TestSettings> settings1(MakeTestSettings());

    settings1.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings1.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));

    Settings<TestSettings> settings2(MakeTestSettings());

    settings2.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings2.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings2.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings2.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));

    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));

    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 1000);
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));

    // No diff
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));

    // Diff
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Tesz!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting4_string));

    // No diff
    settings2.SetValue<std::string>(TestSettings::Setting4_string, std::string("Tesz!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
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

    fSetting.ClearDirty();

    e.Pull(fSetting);

    EXPECT_EQ(4.0f, fSetting.GetValue());
    EXPECT_TRUE(fSetting.IsDirty());
}
