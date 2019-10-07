#include <GameCore/Settings.h>

#include <GameCore/Utils.h>

#include "Utils.h"

#include <picojson.h>

#include "gtest/gtest.h"

/////////////////////////////////////////////
// Custom type and its serialization
/////////////////////////////////////////////

struct CustomValue
{
    std::string Str;
    int Int;

    CustomValue()
    {}

    CustomValue(
        std::string const &str,
        int _int)
        : Str(str)
        , Int(_int)
    {}

    bool operator==(CustomValue const & other) const
    {
        return Str == other.Str
            && Int == other.Int;
    }
};

template<>
void Setting<CustomValue>::Serialize(SettingsSerializationContext & context) const
{
    auto os = context.GetNamedStream(GetName(), "bin");
    *os << mValue.Str + ":" + std::to_string(mValue.Int);
}

template<>
void Setting<CustomValue>::Deserialize(SettingsDeserializationContext const & context)
{
    auto const is = context.GetNamedStream(GetName(), "bin");

    std::string tmp;
    *is >> tmp;

    auto pos = tmp.find(':');
    assert(pos != std::string::npos);

    mValue.Str = tmp.substr(0, pos);
    mValue.Int = std::stoi(tmp.substr(pos + 1));
        
    MarkAsDirty();
}

/////////////////////////////////////////////

enum TestSettings : size_t
{
    Setting1_float = 0,
    Setting2_uint32,
    Setting3_bool,
    Setting4_string,
    Setting5_custom,

    _Last = Setting5_custom
};

auto MakeTestSettings()
{
    std::vector<std::unique_ptr<BaseSetting>> settings;
    settings.emplace_back(new Setting<float>("setting1_float"));
    settings.emplace_back(new Setting<uint32_t>("setting2_uint32"));
    settings.emplace_back(new Setting<bool>("setting3_bool"));
    settings.emplace_back(new Setting<std::string>("setting4_string"));
    settings.emplace_back(new Setting<CustomValue>("setting5_custom"));

    return settings;
}

static std::filesystem::path TestRootSystemDirectory = "C:\\Foo\\System\\";
static std::filesystem::path TestRootUserDirectory = "C:\\Foo\\User\\";

////////////////////////////////////////////////////////////////

TEST(SettingsTests, Setting_DefaultConstructor)
{
    Setting<float> fSetting("");

    EXPECT_EQ(0.0f, fSetting.GetValue());
    EXPECT_FALSE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_ConstructorValue)
{
    Setting<float> fSetting("", 5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_FALSE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_SetValue)
{
    Setting<float> fSetting("");

    fSetting.SetValue(5.0f);

    EXPECT_EQ(5.0f, fSetting.GetValue());
    EXPECT_TRUE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_MarkAsDirty)
{
    Setting<float> fSetting("");

    fSetting.ClearDirty();

    EXPECT_FALSE(fSetting.IsDirty());

    fSetting.MarkAsDirty();

    EXPECT_TRUE(fSetting.IsDirty());
}

TEST(SettingsTests, Setting_ClearDirty)
{
    Setting<float> fSetting("");

    fSetting.MarkAsDirty();

    EXPECT_TRUE(fSetting.IsDirty());

    fSetting.ClearDirty();

    EXPECT_FALSE(fSetting.IsDirty());
}
TEST(SettingsTests, Setting_Type)
{
    Setting<float> fSetting("");

    EXPECT_EQ(typeid(float), fSetting.GetType());
}

TEST(SettingsTests, Setting_IsEqual)
{
    Setting<float> fSetting1("");
    fSetting1.SetValue(5.0f);

    Setting<float> fSetting2("");
    fSetting2.SetValue(15.0f);

    Setting<float> fSetting3("");
    fSetting3.SetValue(5.0f);

    EXPECT_FALSE(fSetting1.IsEqual(fSetting2));
    EXPECT_TRUE(fSetting1.IsEqual(fSetting3));
}

TEST(SettingsTests, Setting_Clone)
{
    Setting<float> fSetting("");
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

    settings.SetValue(TestSettings::Setting1_float, float(242.0f));
    settings.SetValue(TestSettings::Setting2_uint32, uint32_t(999));
    settings.SetValue(TestSettings::Setting3_bool, bool(true));
    settings.SetValue(TestSettings::Setting4_string, std::string("Test!"));
    settings.SetValue(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    EXPECT_EQ(242.0f, settings.GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(999u, settings.GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(true, settings.GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("Test!"), settings.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("Foo"), settings.GetValue<CustomValue>(TestSettings::Setting5_custom).Str);
    EXPECT_EQ(123, settings.GetValue<CustomValue>(TestSettings::Setting5_custom).Int);
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
    settings1.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));
    
    Settings<TestSettings> settings2(MakeTestSettings());

    settings2.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings2.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings2.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings2.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings2.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting5_custom));

    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 1000);
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting5_custom));

    settings1.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Bar", 123));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting5_custom));

    // No diff
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting5_custom));

    // Diff
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Tesz!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting5_custom));

    // No diff
    settings2.SetValue<std::string>(TestSettings::Setting4_string, std::string("Tesz!"));
    settings1.SetDirtyWithDiff(settings2);

    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings1.IsDirty(TestSettings::Setting4_string));
    EXPECT_TRUE(settings1.IsDirty(TestSettings::Setting5_custom));
}

/////////////////////////////////////////////////////////

TEST(SettingsTests, Storage_EnsuresUserSettingsDirectoryExists)
{
    ::testing::InSequence dummy;

    std::shared_ptr<MockFileSystem> mockFileSystem(new ::testing::StrictMock<MockFileSystem>());

    EXPECT_CALL(*mockFileSystem, EnsureDirectoryExists(TestRootUserDirectory))
        .Times(1);

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        mockFileSystem);
}

TEST(SettingsTests, Storage_DeleteAllFilesDeletesAllStreamsAndSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    testFileSystem->GetFileMap()[TestRootUserDirectory / "Test Name.settings.json"] = std::make_shared<memory_streambuf>();
    testFileSystem->GetFileMap()[TestRootUserDirectory / "Test Name.foo bar.dat"] = std::make_shared<memory_streambuf>();
    testFileSystem->GetFileMap()[TestRootUserDirectory / "Test Namez.yulp.abracadabra"] = std::make_shared<memory_streambuf>();
    testFileSystem->GetFileMap()[TestRootUserDirectory / "Test Name.yulp.abracadabra"] = std::make_shared<memory_streambuf>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);

    EXPECT_EQ(4, testFileSystem->GetFileMap().size());

    storage.DeleteAllFiles(PersistedSettingsKey("Test Name", StorageTypes::User));

    ASSERT_EQ(1, testFileSystem->GetFileMap().size());
    EXPECT_EQ(1, testFileSystem->GetFileMap().count(TestRootUserDirectory / "Test Namez.yulp.abracadabra"));
}

/////////////////////////////////////////////////////////

TEST(SettingsTests, Serialization_Settings_AllDirty)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    std::shared_ptr<SettingsStorage> storage = std::make_shared<SettingsStorage>(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings(MakeTestSettings());

    settings.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Bar", 123));

    settings.MarkAllAsDirty();

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 0);

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", StorageTypes::User),
            storage);

        settings.SerializeDirty(sContext);
        // Context destruction happens here
    }

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 2);

    //
    // Verify json
    //

    std::filesystem::path expectedJsonSettingsFilePath = TestRootUserDirectory / "Test Settings.settings.json";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedJsonSettingsFilePath), 1);

    std::string jsonSettingsContent = std::string(
        testFileSystem->GetFileMap()[expectedJsonSettingsFilePath]->data(),
        testFileSystem->GetFileMap()[expectedJsonSettingsFilePath]->size());

    auto settingsRootValue = Utils::ParseJSONString(jsonSettingsContent);
    ASSERT_TRUE(settingsRootValue.is<picojson::object>());

    auto & settingsRootObject = settingsRootValue.get<picojson::object>();
    EXPECT_EQ(2, settingsRootObject.size());

    // Version
    ASSERT_EQ(1, settingsRootObject.count("version"));
    ASSERT_TRUE(settingsRootObject["version"].is<std::string>());
    EXPECT_EQ(Version::CurrentVersion().ToString(), settingsRootObject["version"].get<std::string>());

    // Settings
    ASSERT_EQ(1, settingsRootObject.count("settings"));
    ASSERT_TRUE(settingsRootObject["settings"].is<picojson::object>());

    auto & settingsObject = settingsRootObject["settings"].get<picojson::object>();

    // Settings content

    EXPECT_EQ(4, settingsObject.size());

    ASSERT_EQ(1, settingsObject.count("setting1_float"));
    ASSERT_TRUE(settingsObject["setting1_float"].is<double>());
    EXPECT_DOUBLE_EQ(242.0, settingsObject["setting1_float"].get<double>());

    ASSERT_EQ(1, settingsObject.count("setting2_uint32"));
    ASSERT_TRUE(settingsObject["setting2_uint32"].is<int64_t>());
    EXPECT_EQ(999LL, settingsObject["setting2_uint32"].get<int64_t>());

    ASSERT_EQ(1, settingsObject.count("setting3_bool"));
    ASSERT_TRUE(settingsObject["setting3_bool"].is<bool>());
    EXPECT_EQ(true, settingsObject["setting3_bool"].get<bool>());

    ASSERT_EQ(1, settingsObject.count("setting4_string"));
    ASSERT_TRUE(settingsObject["setting4_string"].is<std::string>());
    EXPECT_EQ(std::string("Test!"), settingsObject["setting4_string"].get<std::string>());

    //
    // Custom type named stream
    //

    std::filesystem::path expectedCustomTypeSettingsFilePath = TestRootUserDirectory / "Test Settings.setting5_custom.bin";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedCustomTypeSettingsFilePath), 1);

    std::string customSettingContent = std::string(
        testFileSystem->GetFileMap()[expectedCustomTypeSettingsFilePath]->data(),
        testFileSystem->GetFileMap()[expectedCustomTypeSettingsFilePath]->size());

    EXPECT_EQ(std::string("Bar:123"), customSettingContent);
}

TEST(SettingsTests, Serialization_Settings_AllClean)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    std::shared_ptr<SettingsStorage> storage = std::make_shared<SettingsStorage>(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings(MakeTestSettings());

    settings.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.SetValue<bool>(TestSettings::Setting3_bool, true);
    settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Bar", 123));

    settings.ClearAllDirty();

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", StorageTypes::User),
            storage);

        settings.SerializeDirty(sContext);
        // Context destruction happens here
    }

    //
    // Verify json
    //

    std::filesystem::path expectedJsonSettingsFilePath = TestRootUserDirectory / "Test Settings.settings.json";

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 1);
    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedJsonSettingsFilePath), 1);

    std::string settingsContent = std::string(
        testFileSystem->GetFileMap()[expectedJsonSettingsFilePath]->data(),
        testFileSystem->GetFileMap()[expectedJsonSettingsFilePath]->size());

    auto settingsRootValue = Utils::ParseJSONString(settingsContent);
    ASSERT_TRUE(settingsRootValue.is<picojson::object>());

    auto & settingsRootObject = settingsRootValue.get<picojson::object>();
    EXPECT_EQ(2, settingsRootObject.size());

    // Version
    ASSERT_EQ(1, settingsRootObject.count("version"));
    ASSERT_TRUE(settingsRootObject["version"].is<std::string>());
    EXPECT_EQ(Version::CurrentVersion().ToString(), settingsRootObject["version"].get<std::string>());

    // Settings
    ASSERT_EQ(1, settingsRootObject.count("settings"));
    ASSERT_TRUE(settingsRootObject["settings"].is<picojson::object>());

    auto & settingsObject = settingsRootObject["settings"].get<picojson::object>();
    EXPECT_EQ(0, settingsObject.size());
}

/* TODOHERE
TEST(SettingsTests, Serialization_SerializesOnlyDirtySettings)
{
    // TODOHERE: both setting and named stream
}

TEST(SettingsTests, Serialization_DeserializedSettingsAreMarkedAsDirty)
{
    // TODOHERE
}
*/

/////////////////////////////////////////////////////////

TEST(SettingsTests, Enforcer_Enforce)
{
    Setting<float> fSetting("");
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
    Setting<float> fSetting("");
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
