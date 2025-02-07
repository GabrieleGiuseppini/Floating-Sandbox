#include <Game/Settings.h>

#include <Core/Utils.h>

#include "TestingUtils.h"

#include <picojson.h>

#include <algorithm>

#include "gtest/gtest.h"

// Custom type and its serialization

struct CustomValue
{
    std::string Str;
    int Int;

    CustomValue()
    {}

    CustomValue(
        std::string const & str,
        int _int)
        : Str(str)
        , Int(_int)
    {}

    CustomValue(CustomValue const & other) = default;
    CustomValue(CustomValue && other) = default;
    CustomValue & operator=(CustomValue const & other) = default;
    CustomValue & operator=(CustomValue && other) = default;

    bool operator==(CustomValue const & other) const
    {
        return Str == other.Str
            && Int == other.Int;
    }
};

template<>
void SettingSerializer::Serialize<CustomValue>(
    SettingsSerializationContext & context,
    std::string const & settingName,
    CustomValue const & value)
{
    auto os = context.GetNamedTextOutputStream(settingName, "bin");
    os->Write(value.Str + ":" + std::to_string(value.Int));
}

template<>
bool SettingSerializer::Deserialize<CustomValue>(
    SettingsDeserializationContext const & context,
    std::string const & settingName,
    CustomValue & value)
{
    auto const is = context.GetNamedTextInputStream(settingName, "bin");
    if (!!is)
    {
        std::string tmp = is->ReadAll();

        auto pos = tmp.find(':');
        assert(pos != std::string::npos);

        value.Str = tmp.substr(0, pos);
        value.Int = std::stoi(tmp.substr(pos + 1));

        return true;
    }

    return false;
}

// Test template settings

enum class TestSettings : size_t
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

static std::filesystem::path TestRootSystemDirectory = "C:\\Foo\\System";
static std::filesystem::path TestRootUserDirectory = "C:\\Foo\\User";

////////////////////////////////////////////////////////////////
// Setting
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

////////////////////////////////////////////////////////////////
// Settings
////////////////////////////////////////////////////////////////

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

TEST(SettingsTests, Settings_SetAndGetValue_ByConstRef)
{
    Settings<TestSettings> settings(MakeTestSettings());

    std::string const testVal = "Test!";
    settings.SetValue(TestSettings::Setting4_string, testVal);

    EXPECT_EQ(std::string("Test!"), settings.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("Test!"), testVal);
}

TEST(SettingsTests, Settings_SetAndGetValue_ByRValue)
{
    Settings<TestSettings> settings(MakeTestSettings());

    std::string testVal = "Test!";
    settings.SetValue(TestSettings::Setting4_string, std::move(testVal));

    EXPECT_EQ(std::string("Test!"), settings.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string(""), testVal);
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

TEST(SettingsTests, Settings_Comparison)
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

    Settings<TestSettings> settings3(MakeTestSettings());
	settings3.SetValue<float>(TestSettings::Setting1_float, 242.0f);
	settings3.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
	settings3.SetValue<bool>(TestSettings::Setting3_bool, true);
	settings3.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
	settings3.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    settings2.SetValue(TestSettings::Setting5_custom, CustomValue("Foo", 124));

    EXPECT_EQ(settings1, settings3);
    EXPECT_NE(settings1, settings2);
}


////////////////////////////////////////////////////////////////
// Storage
////////////////////////////////////////////////////////////////

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

TEST(SettingsTests, Storage_DeleteDeletesAllStreamsAndSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name.settings.json");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name.foo bar.dat");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Namez.yulp.abracadabra");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name.yulp.abracadabra");

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);

    ASSERT_EQ(4u, testFileSystem->GetFileMap().size());

    storage.Delete(PersistedSettingsKey("Test Name", PersistedSettingsStorageTypes::User));

    ASSERT_EQ(1u, testFileSystem->GetFileMap().size());
    EXPECT_EQ(1u, testFileSystem->GetFileMap().count(TestRootUserDirectory / "Test Namez.yulp.abracadabra"));
}

TEST(SettingsTests, Storage_ListSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    std::string const testJson1 = R"({"version":"1.2.3.4","description":"This is a description","settings":{}})";
    std::string const testJson2 = R"({"version":"1.2.3.4","description":"","settings":{}})";

    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 1.settings.json", testJson2);
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 1.foo bar.dat");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Hidden Settings.yulp.abracadabra");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name.yulp.abracadabra");
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Super Settings.settings.json", testJson1);
    testFileSystem->PrepareTestFile(TestRootSystemDirectory / "System Settings.settings.json", testJson2);
    testFileSystem->PrepareTestFile(TestRootSystemDirectory / "System Settings.yulp.abracadabra");
    testFileSystem->PrepareTestFile(TestRootSystemDirectory / "System Hidden Settings.yulp.abracadabra");

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);

    auto settings = storage.ListSettings();

    ASSERT_EQ(3u, settings.size());

    std::sort(
        settings.begin(),
        settings.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Key.Name < rhs.Key.Name;
        });

    EXPECT_EQ(settings[0].Key, PersistedSettingsKey("Super Settings", PersistedSettingsStorageTypes::User));
    EXPECT_EQ(settings[0].Description, "This is a description");

    EXPECT_EQ(settings[1].Key, PersistedSettingsKey("System Settings", PersistedSettingsStorageTypes::System));
    EXPECT_EQ(settings[1].Description, "");

    EXPECT_EQ(settings[2].Key, PersistedSettingsKey("Test Name 1", PersistedSettingsStorageTypes::User));
    EXPECT_EQ(settings[2].Description, "");
}

////////////////////////////////////////////////////////////////
// Serialization
////////////////////////////////////////////////////////////////

TEST(SettingsTests, Serialization_Settings_AllDirty)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
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

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 0u);

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings.SerializeDirty(sContext);
        // Context destruction happens here
    }

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 2u);

    //
    // Verify json
    //

    std::filesystem::path expectedJsonSettingsFilePath = TestRootUserDirectory / "Test Settings.settings.json";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedJsonSettingsFilePath), 1u);

    std::string jsonSettingsContent = testFileSystem->GetTestFileContent(expectedJsonSettingsFilePath);

    auto settingsRootValue = Utils::ParseJSONString(jsonSettingsContent);
    ASSERT_TRUE(settingsRootValue.is<picojson::object>());

    auto & settingsRootObject = settingsRootValue.get<picojson::object>();
    EXPECT_EQ(3u, settingsRootObject.size());

    // Version
    ASSERT_EQ(1u, settingsRootObject.count("version"));
    ASSERT_TRUE(settingsRootObject["version"].is<std::string>());
    EXPECT_EQ(CurrentGameVersion.ToString(), settingsRootObject["version"].get<std::string>());

    // Description
    ASSERT_EQ(1u, settingsRootObject.count("description"));
    ASSERT_TRUE(settingsRootObject["description"].is<std::string>());
    EXPECT_EQ(std::string("Test description"), settingsRootObject["description"].get<std::string>());

    // Settings
    ASSERT_EQ(1u, settingsRootObject.count("settings"));
    ASSERT_TRUE(settingsRootObject["settings"].is<picojson::object>());

    auto & settingsObject = settingsRootObject["settings"].get<picojson::object>();

    // Settings content

    EXPECT_EQ(4u, settingsObject.size());

    ASSERT_EQ(1u, settingsObject.count("setting1_float"));
    ASSERT_TRUE(settingsObject["setting1_float"].is<double>());
    EXPECT_DOUBLE_EQ(242.0, settingsObject["setting1_float"].get<double>());

    ASSERT_EQ(1u, settingsObject.count("setting2_uint32"));
    ASSERT_TRUE(settingsObject["setting2_uint32"].is<int64_t>());
    EXPECT_EQ(999LL, settingsObject["setting2_uint32"].get<int64_t>());

    ASSERT_EQ(1u, settingsObject.count("setting3_bool"));
    ASSERT_TRUE(settingsObject["setting3_bool"].is<bool>());
    EXPECT_EQ(true, settingsObject["setting3_bool"].get<bool>());

    ASSERT_EQ(1u, settingsObject.count("setting4_string"));
    ASSERT_TRUE(settingsObject["setting4_string"].is<std::string>());
    EXPECT_EQ(std::string("Test!"), settingsObject["setting4_string"].get<std::string>());

    //
    // Custom type named stream
    //

    std::filesystem::path expectedCustomTypeSettingsFilePath = TestRootUserDirectory / "Test Settings.setting5_custom.bin";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedCustomTypeSettingsFilePath), 1u);

    std::string customSettingContent = testFileSystem->GetTestFileContent(expectedCustomTypeSettingsFilePath);

    EXPECT_EQ(std::string("Bar:123"), customSettingContent);
}

TEST(SettingsTests, Serialization_Settings_AllClean)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
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
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings.SerializeDirty(sContext);
        // Context destruction happens here
    }

    //
    // Verify json
    //

    std::filesystem::path expectedJsonSettingsFilePath = TestRootUserDirectory / "Test Settings.settings.json";

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 1u);
    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedJsonSettingsFilePath), 1u);

    std::string settingsContent = testFileSystem->GetTestFileContent(expectedJsonSettingsFilePath);

    auto settingsRootValue = Utils::ParseJSONString(settingsContent);
    ASSERT_TRUE(settingsRootValue.is<picojson::object>());

    auto & settingsRootObject = settingsRootValue.get<picojson::object>();
    EXPECT_EQ(3u, settingsRootObject.size());

    // Version
    ASSERT_EQ(1u, settingsRootObject.count("version"));
    ASSERT_TRUE(settingsRootObject["version"].is<std::string>());
    EXPECT_EQ(CurrentGameVersion.ToString(), settingsRootObject["version"].get<std::string>());

    // Description
    ASSERT_EQ(1u, settingsRootObject.count("description"));
    ASSERT_TRUE(settingsRootObject["description"].is<std::string>());
    EXPECT_EQ(std::string("Test description"), settingsRootObject["description"].get<std::string>());

    // Settings
    ASSERT_EQ(1u, settingsRootObject.count("settings"));
    ASSERT_TRUE(settingsRootObject["settings"].is<picojson::object>());

    auto & settingsObject = settingsRootObject["settings"].get<picojson::object>();
    EXPECT_EQ(0u, settingsObject.size());
}

TEST(SettingsTests, Serialization_SerializesOnlyDirtySettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings(MakeTestSettings());

    settings.ClearAllDirty();

    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Bar", 123));

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 0u);

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings.SerializeDirty(sContext);
        // Context destruction happens here
    }

    EXPECT_EQ(testFileSystem->GetFileMap().size(), 2u);

    //
    // Verify json
    //

    std::filesystem::path expectedJsonSettingsFilePath = TestRootUserDirectory / "Test Settings.settings.json";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedJsonSettingsFilePath), 1u);

    std::string jsonSettingsContent = testFileSystem->GetTestFileContent(expectedJsonSettingsFilePath);

    auto settingsRootValue = Utils::ParseJSONString(jsonSettingsContent);
    ASSERT_TRUE(settingsRootValue.is<picojson::object>());

    auto & settingsRootObject = settingsRootValue.get<picojson::object>();
    EXPECT_EQ(3u, settingsRootObject.size());

    // Version
    ASSERT_EQ(1u, settingsRootObject.count("version"));
    ASSERT_TRUE(settingsRootObject["version"].is<std::string>());
    EXPECT_EQ(CurrentGameVersion.ToString(), settingsRootObject["version"].get<std::string>());

    // Description
    ASSERT_EQ(1u, settingsRootObject.count("description"));
    ASSERT_TRUE(settingsRootObject["description"].is<std::string>());
    EXPECT_EQ(std::string("Test description"), settingsRootObject["description"].get<std::string>());

    // Settings
    ASSERT_EQ(1u, settingsRootObject.count("settings"));
    ASSERT_TRUE(settingsRootObject["settings"].is<picojson::object>());

    auto & settingsObject = settingsRootObject["settings"].get<picojson::object>();

    // Settings content

    EXPECT_EQ(1u, settingsObject.size());
    EXPECT_EQ(1u, settingsObject.count("setting2_uint32"));

    //
    // Custom type named stream
    //

    std::filesystem::path expectedCustomTypeSettingsFilePath = TestRootUserDirectory / "Test Settings.setting5_custom.bin";

    ASSERT_EQ(testFileSystem->GetFileMap().count(expectedCustomTypeSettingsFilePath), 1u);
}

TEST(SettingsTests, Serialization_E2E_SerializationAndDeserialization)
{
    //
    // 1. Serialize
    //

    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings1(MakeTestSettings());

    settings1.ClearAllDirty();

    settings1.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings1.SetValue<bool>(TestSettings::Setting3_bool, false);
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings1.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings1.SerializeDirty(sContext);
        // Context destruction happens here
    }


    //
    // 2. De-serialize
    //

    Settings<TestSettings> settings2(MakeTestSettings());

    settings2.MarkAllAsDirty();

    {
        SettingsDeserializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            storage);

        settings2.Deserialize(sContext);
    }


    //
    // 3. Verify
    //

    EXPECT_EQ(242.0f, settings2.GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(999u, settings2.GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(false, settings2.GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("Test!"), settings2.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("Foo"), settings2.GetValue<CustomValue>(TestSettings::Setting5_custom).Str);
    EXPECT_EQ(123, settings2.GetValue<CustomValue>(TestSettings::Setting5_custom).Int);
}

enum class RgbColorTestSettings : size_t
{
    Setting1 = 0,
    _Last = Setting1
};

TEST(SettingsTests, Serialization_E2E_SerializationAndDeserialization_rgbColor)
{
    //
    // 1. Serialize
    //

    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);

    std::vector<std::unique_ptr<BaseSetting>> testSettings;
    testSettings.emplace_back(new Setting<rgbColor>("setting1"));

    Settings<RgbColorTestSettings> settings1(testSettings);
    settings1.ClearAllDirty();

    settings1.SetValue<rgbColor>(RgbColorTestSettings::Setting1, rgbColor(22, 33, 45));

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings1.SerializeDirty(sContext);
        // Context destruction happens here
    }


    //
    // 2. De-serialize
    //

    Settings<RgbColorTestSettings> settings2(testSettings);
    settings2.MarkAllAsDirty();

    {
        SettingsDeserializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            storage);

        settings2.Deserialize(sContext);
    }


    //
    // 3. Verify
    //

    EXPECT_EQ(rgbColor(22, 33, 45), settings2.GetValue<rgbColor>(RgbColorTestSettings::Setting1));
}

enum class TestEnum : int
{
    Value1,
    Value2,
    Value3,
    Value4,
    Value5,
    Value6
};

enum class EnumTestSettings : size_t
{
    Setting1 = 0,
    Setting2,
    Setting3,
    _Last = Setting3
};

TEST(SettingsTests, Serialization_E2E_SerializationAndDeserialization_Enum)
{
    //
    // 1. Serialize
    //

    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);

    std::vector<std::unique_ptr<BaseSetting>> testSettings;
    testSettings.emplace_back(new Setting<TestEnum>("setting1"));
    testSettings.emplace_back(new Setting<TestEnum>("setting2"));
    testSettings.emplace_back(new Setting<TestEnum>("setting3"));

    Settings<EnumTestSettings> settings1(testSettings);
    settings1.ClearAllDirty();

    settings1.SetValue<TestEnum>(EnumTestSettings::Setting1, TestEnum::Value2);
    settings1.SetValue<TestEnum>(EnumTestSettings::Setting2, TestEnum::Value4);
    settings1.SetValue<TestEnum>(EnumTestSettings::Setting3, TestEnum::Value5);

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings1.SerializeDirty(sContext);
        // Context destruction happens here
    }


    //
    // 2. De-serialize
    //

    Settings<EnumTestSettings> settings2(testSettings);
    settings2.MarkAllAsDirty();

    {
        SettingsDeserializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            storage);

        settings2.Deserialize(sContext);
    }


    //
    // 3. Verify
    //

    EXPECT_EQ(TestEnum::Value2, settings2.GetValue<TestEnum>(EnumTestSettings::Setting1));
    EXPECT_EQ(TestEnum::Value4, settings2.GetValue<TestEnum>(EnumTestSettings::Setting2));
    EXPECT_EQ(TestEnum::Value5, settings2.GetValue<TestEnum>(EnumTestSettings::Setting3));
}

enum class ChronoTestSettings : size_t
{
	Setting1 = 0,
	Setting2 = 1,
	_Last = Setting2
};

TEST(SettingsTests, Serialization_E2E_SerializationAndDeserialization_chrono)
{
	//
	// 1. Serialize
	//

	auto testFileSystem = std::make_shared<TestFileSystem>();

	SettingsStorage storage(
		TestRootSystemDirectory,
		TestRootUserDirectory,
		testFileSystem);

	std::vector<std::unique_ptr<BaseSetting>> testSettings;
	testSettings.emplace_back(new Setting<std::chrono::seconds>("setting1"));
	testSettings.emplace_back(new Setting<std::chrono::minutes>("setting2"));

	Settings<ChronoTestSettings> settings1(testSettings);
	settings1.ClearAllDirty();

	settings1.SetValue<std::chrono::seconds>(ChronoTestSettings::Setting1, std::chrono::seconds(7));
	settings1.SetValue<std::chrono::minutes>(ChronoTestSettings::Setting2, std::chrono::minutes(42));

	{
		SettingsSerializationContext sContext(
			PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
			"Test description",
			storage);

		settings1.SerializeDirty(sContext);
		// Context destruction happens here
	}


	//
	// 2. De-serialize
	//

	Settings<ChronoTestSettings> settings2(testSettings);
	settings2.MarkAllAsDirty();

	{
		SettingsDeserializationContext sContext(
			PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
			storage);

		settings2.Deserialize(sContext);
	}


	//
	// 3. Verify
	//

	EXPECT_EQ(std::chrono::seconds(7), settings2.GetValue<std::chrono::seconds>(ChronoTestSettings::Setting1));
	EXPECT_EQ(std::chrono::minutes(42), settings2.GetValue<std::chrono::minutes>(ChronoTestSettings::Setting2));
}

TEST(SettingsTests, Serialization_DeserializedSettingsAreMarkedAsDirty)
{
    //
    // 1. Serialize
    //

    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings1(MakeTestSettings());

    settings1.ClearAllDirty();

    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings1.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Bar", 123));

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings1.SerializeDirty(sContext);
        // Context destruction happens here
    }


    //
    // 2. De-serialize
    //

    Settings<TestSettings> settings2(MakeTestSettings());

    settings2.MarkAllAsDirty();

    {
        SettingsDeserializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            storage);

        settings2.Deserialize(sContext);
    }


    //
    // 3. Verify
    //

    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings2.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting4_string));
    EXPECT_TRUE(settings2.IsDirty(TestSettings::Setting5_custom));
}

TEST(SettingsTests, Serialization_CustomNonDeserializedSettingIsClean)
{
    //
    // 1. Serialize
    //

    auto testFileSystem = std::make_shared<TestFileSystem>();

    SettingsStorage storage(
        TestRootSystemDirectory,
        TestRootUserDirectory,
        testFileSystem);


    Settings<TestSettings> settings1(MakeTestSettings());

    settings1.ClearAllDirty();

    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);

    {
        SettingsSerializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            "Test description",
            storage);

        settings1.SerializeDirty(sContext);
        // Context destruction happens here
    }


    //
    // 2. De-serialize
    //

    Settings<TestSettings> settings2(MakeTestSettings());

    settings2.MarkAllAsDirty();

    {
        SettingsDeserializationContext sContext(
            PersistedSettingsKey("Test Settings", PersistedSettingsStorageTypes::User),
            storage);

        settings2.Deserialize(sContext);
    }


    //
    // 3. Verify
    //

    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting1_float));
    EXPECT_TRUE(settings2.IsDirty(TestSettings::Setting2_uint32));
    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting3_bool));
    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting4_string));
    EXPECT_FALSE(settings2.IsDirty(TestSettings::Setting5_custom));
}

////////////////////////////////////////////////////////////////
// Enforcer
////////////////////////////////////////////////////////////////

TEST(SettingsTests, Enforcer_Enforce)
{
    Setting<float> fSetting("");
    fSetting.SetValue(5.0f);

    float valueBeingSet = 20.0f;
	float valueBeingSetImmediate = 20.0f;

    SettingEnforcer<float> e(
        [&valueBeingSet]() -> float const &
        {
            return valueBeingSet;
        },
        [&valueBeingSet](float const & value)
        {
            valueBeingSet = value;
        },
		[&valueBeingSetImmediate](float const & value)
		{
			valueBeingSetImmediate = value;
		});

    e.Enforce(fSetting);

    EXPECT_EQ(valueBeingSet, 5.0f);
	EXPECT_EQ(valueBeingSetImmediate, 20.0f);

	valueBeingSet = 20.0f;

	e.EnforceImmediate(fSetting);

	EXPECT_EQ(valueBeingSet, 20.0f);
	EXPECT_EQ(valueBeingSetImmediate, 5.0f);
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

////////////////////////////////////////////////////////////////
// BaseSettingsManager
////////////////////////////////////////////////////////////////

// Mimics the place where the enforcers enforce to/pull from
struct TestGlobalSettings
{
    float setting1{ 0.0f };
    uint32_t setting2{ 45 };
    bool setting3{ false };
    std::string setting4{ "" };
    CustomValue setting5{ "", 45 };
};

static TestGlobalSettings GlobalSettings;
static TestGlobalSettings GlobalSettingsImmediate;

class TestSettingsManager : public BaseSettingsManager<TestSettings, TestFileSystem>
{
public:

    static BaseSettingsManagerFactory MakeSettingsFactory()
    {
        BaseSettingsManagerFactory factory;

        factory.AddSetting<float>(
            TestSettings::Setting1_float,
            "setting1_float",
            []() -> float { return GlobalSettings.setting1; },
            [](auto const & v) { GlobalSettings.setting1 = v; },
			[](auto const & v) { GlobalSettingsImmediate.setting1 = v; });

        factory.AddSetting<uint32_t>(
            TestSettings::Setting2_uint32,
            "setting2_uint32",
            []() -> uint32_t { return GlobalSettings.setting2; },
            [](auto const & v) { GlobalSettings.setting2 = v; },
			[](auto const & v) { GlobalSettingsImmediate.setting2 = v; });

        factory.AddSetting<bool>(
            TestSettings::Setting3_bool,
            "setting3_bool",
            []() -> bool { return GlobalSettings.setting3; },
            [](auto const & v) { GlobalSettings.setting3 = v; },
			[](auto const & v) { GlobalSettingsImmediate.setting3 = v; });

        factory.AddSetting<std::string>(
            TestSettings::Setting4_string,
            "setting4_string",
            []() -> std::string { return GlobalSettings.setting4; },
            [](auto const & v) { GlobalSettings.setting4 = v; },
			[](auto const & v) { GlobalSettingsImmediate.setting4 = v; });

        factory.AddSetting<CustomValue>(
            TestSettings::Setting5_custom,
            "setting5_custom",
            []() -> CustomValue { return GlobalSettings.setting5; },
            [](auto const & v) { GlobalSettings.setting5 = v; },
			[](auto const & v) { GlobalSettingsImmediate.setting5 = v; });

        return factory;
    }

    TestSettingsManager(std::shared_ptr<TestFileSystem> fileSystem)
        : BaseSettingsManager(
            MakeSettingsFactory(),
            TestRootSystemDirectory,
            TestRootUserDirectory,
            std::move(fileSystem))
    {}
};

TEST(SettingsTests, BaseSettingsManager_BuildsDefaults)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    // Set defaults
    GlobalSettings.setting1 = 789.5f;
    GlobalSettings.setting2 = 242;
    GlobalSettings.setting3 = true;
    GlobalSettings.setting4 = "A Forest";
    GlobalSettings.setting5 = CustomValue("MyVal", 50);

    // Create manager - defaults are taken at this moment
    TestSettingsManager sm(testFileSystem);

    // Verify defaults
    EXPECT_EQ(789.5f, sm.GetDefaults().GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(242u, sm.GetDefaults().GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(true, sm.GetDefaults().GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("A Forest"), sm.GetDefaults().GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("MyVal"), sm.GetDefaults().GetValue<CustomValue>(TestSettings::Setting5_custom).Str);
    EXPECT_EQ(50, sm.GetDefaults().GetValue<CustomValue>(TestSettings::Setting5_custom).Int);
}

TEST(SettingsTests, BaseSettingsManager_Enforces)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();
    TestSettingsManager sm(testFileSystem);

    // Set baseline
    GlobalSettings.setting1 = 789.5f;
    GlobalSettings.setting2 = 242;
    GlobalSettings.setting3 = true;
    GlobalSettings.setting4 = "A Forest";
    GlobalSettings.setting5 = CustomValue("MyVal", 50);

	// Set baseline immediate
	GlobalSettingsImmediate.setting1 = 0.0f;
	GlobalSettingsImmediate.setting2 = 0;
	GlobalSettingsImmediate.setting3 = false;
	GlobalSettingsImmediate.setting4 = "";
	GlobalSettingsImmediate.setting5 = CustomValue("", 0);

    // Prepare settings
    Settings<TestSettings> settings(MakeTestSettings());
    settings.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings.ClearAllDirty();
    // Only the following will be enforced
    settings.SetValue<bool>(TestSettings::Setting3_bool, false);
    settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    // Enforce dirty
    sm.EnforceDirtySettings(settings);

    // Verify
    EXPECT_EQ(789.5f, GlobalSettings.setting1);
    EXPECT_EQ(242u, GlobalSettings.setting2);
    EXPECT_EQ(false, GlobalSettings.setting3);
    EXPECT_EQ(std::string("Test!"), GlobalSettings.setting4);
    EXPECT_EQ(std::string("Foo"), GlobalSettings.setting5.Str);
    EXPECT_EQ(123, GlobalSettings.setting5.Int);

	// Verify immediate are left where they were
	EXPECT_EQ(0.0f, GlobalSettingsImmediate.setting1);
	EXPECT_EQ(0u, GlobalSettingsImmediate.setting2);
	EXPECT_EQ(false, GlobalSettingsImmediate.setting3);
	EXPECT_EQ(std::string(""), GlobalSettingsImmediate.setting4);
	EXPECT_EQ(std::string(""), GlobalSettingsImmediate.setting5.Str);
	EXPECT_EQ(0, GlobalSettingsImmediate.setting5.Int);
}

TEST(SettingsTests, BaseSettingsManager_Enforces_Immediate)
{
	auto testFileSystem = std::make_shared<TestFileSystem>();
	TestSettingsManager sm(testFileSystem);

	// Set baseline
	GlobalSettings.setting1 = 789.5f;
	GlobalSettings.setting2 = 242;
	GlobalSettings.setting3 = true;
	GlobalSettings.setting4 = "A Forest";
	GlobalSettings.setting5 = CustomValue("MyVal", 50);

	// Set baseline immediate
	GlobalSettingsImmediate.setting1 = 0.0f;
	GlobalSettingsImmediate.setting2 = 0;
	GlobalSettingsImmediate.setting3 = true;
	GlobalSettingsImmediate.setting4 = "";
	GlobalSettingsImmediate.setting5 = CustomValue("", 0);

	// Prepare settings
	Settings<TestSettings> settings(MakeTestSettings());
	settings.SetValue<float>(TestSettings::Setting1_float, 242.0f);
	settings.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
	settings.ClearAllDirty();
	// Only the following will be enforced
	settings.SetValue<bool>(TestSettings::Setting3_bool, false);
	settings.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
	settings.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

	// Enforce dirty
	sm.EnforceDirtySettingsImmediate(settings);

	// Verify
	EXPECT_EQ(789.5f, GlobalSettings.setting1);
	EXPECT_EQ(242u, GlobalSettings.setting2);
	EXPECT_EQ(true, GlobalSettings.setting3);
	EXPECT_EQ(std::string("A Forest"), GlobalSettings.setting4);
	EXPECT_EQ(std::string("MyVal"), GlobalSettings.setting5.Str);
	EXPECT_EQ(50, GlobalSettings.setting5.Int);

	// Verify immediate
	EXPECT_EQ(0.0f, GlobalSettingsImmediate.setting1);
	EXPECT_EQ(0u, GlobalSettingsImmediate.setting2);
	EXPECT_EQ(false, GlobalSettingsImmediate.setting3);
	EXPECT_EQ(std::string("Test!"), GlobalSettingsImmediate.setting4);
	EXPECT_EQ(std::string("Foo"), GlobalSettingsImmediate.setting5.Str);
	EXPECT_EQ(123, GlobalSettingsImmediate.setting5.Int);
}

TEST(SettingsTests, BaseSettingsManager_Pulls)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();
    TestSettingsManager sm(testFileSystem);

    // Set new values
    GlobalSettings.setting1 = 789.5f;
    GlobalSettings.setting2 = 242;
    GlobalSettings.setting3 = true;
    GlobalSettings.setting4 = "A Forest";
    GlobalSettings.setting5 = CustomValue("MyVal", 50);

    // Pull
    auto settings = sm.Pull();

    // Verify
    EXPECT_EQ(789.5f, settings.GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(242u, settings.GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(true, settings.GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("A Forest"), settings.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("MyVal"), settings.GetValue<CustomValue>(TestSettings::Setting5_custom).Str);
    EXPECT_EQ(50, settings.GetValue<CustomValue>(TestSettings::Setting5_custom).Int);
}

TEST(SettingsTests, BaseSettingsManager_ListPersistedSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    std::string const testJson = R"({"version":"1.2.3.4","description":"","settings":{}})";

    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 1.settings.json", testJson);
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 2.settings.json", testJson);

    TestSettingsManager sm(testFileSystem);

    auto settings = sm.ListPersistedSettings();

    ASSERT_EQ(2u, settings.size());

    std::sort(
        settings.begin(),
        settings.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Key.Name < rhs.Key.Name;
        });

    EXPECT_EQ(settings[0].Key, PersistedSettingsKey("Test Name 1", PersistedSettingsStorageTypes::User));
    EXPECT_EQ(settings[1].Key, PersistedSettingsKey("Test Name 2", PersistedSettingsStorageTypes::User));
}

TEST(SettingsTests, BaseSettingsManager_E2E_SaveAndLoadPersistedSettings_ByVal)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();
    TestSettingsManager sm(testFileSystem);

    //
    // Save settings - make all dirty
    //

    Settings<TestSettings> settings1(MakeTestSettings());
    settings1.SetValue<float>(TestSettings::Setting1_float, 242.0f);
    settings1.SetValue<uint32_t>(TestSettings::Setting2_uint32, 999);
    settings1.SetValue<bool>(TestSettings::Setting3_bool, false);
    settings1.SetValue<std::string>(TestSettings::Setting4_string, std::string("Test!"));
    settings1.SetValue<CustomValue>(TestSettings::Setting5_custom, CustomValue("Foo", 123));

    sm.SaveDirtySettings("TestName", "TestDescription", settings1);

    //
    // Load settings
    //

    Settings<TestSettings> settings2(MakeTestSettings());
	sm.LoadPersistedSettings(
		PersistedSettingsKey("TestName", PersistedSettingsStorageTypes::User),
		settings2);

    //
    // Verify
    //

    EXPECT_EQ(242.0f, settings2.GetValue<float>(TestSettings::Setting1_float));
    EXPECT_EQ(999u, settings2.GetValue<uint32_t>(TestSettings::Setting2_uint32));
    EXPECT_EQ(false, settings2.GetValue<bool>(TestSettings::Setting3_bool));
    EXPECT_EQ(std::string("Test!"), settings2.GetValue<std::string>(TestSettings::Setting4_string));
    EXPECT_EQ(std::string("Foo"), settings2.GetValue<CustomValue>(TestSettings::Setting5_custom).Str);
    EXPECT_EQ(123, settings2.GetValue<CustomValue>(TestSettings::Setting5_custom).Int);
}

TEST(SettingsTests, BaseSettingsManager_E2E_DeletePersistedSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Prepare 3 settings
    //

    std::string const testJson = R"({"version":"1.2.3.4","description":"","settings":{}})";
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 1.settings.json", testJson);
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 2.settings.json", testJson);
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 3.settings.json", testJson);

    TestSettingsManager sm(testFileSystem);

    auto persistedSettings1 = sm.ListPersistedSettings();

    ASSERT_EQ(3u, persistedSettings1.size());

    std::sort(
        persistedSettings1.begin(),
        persistedSettings1.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Key.Name < rhs.Key.Name;
        });

    //
    // Delete 1 setting
    //

    sm.DeletePersistedSettings(persistedSettings1[1].Key);

    //
    // Verify 2 left
    //

    auto persistedSettings2 = sm.ListPersistedSettings();

    EXPECT_EQ(2u, persistedSettings2.size());

    std::sort(
        persistedSettings2.begin(),
        persistedSettings2.end(),
        [](auto const & lhs, auto const & rhs)
        {
            return lhs.Key.Name < rhs.Key.Name;
        });

    EXPECT_EQ(persistedSettings2[0].Key, PersistedSettingsKey("Test Name 1", PersistedSettingsStorageTypes::User));
    EXPECT_EQ(persistedSettings2[1].Key, PersistedSettingsKey("Test Name 3", PersistedSettingsStorageTypes::User));
}

TEST(SettingsTests, BaseSettingsManager_E2E_DeletePersistedSettings_All)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Prepare 3 settings
    //

    std::string const testJson = R"({"version":"1.2.3.4","description":"","settings":{}})";
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 1.settings.json", testJson);
    testFileSystem->PrepareTestFile(TestRootUserDirectory / "Test Name 2.settings.json", testJson);

    TestSettingsManager sm(testFileSystem);

    auto persistedSettings1 = sm.ListPersistedSettings();

    ASSERT_EQ(2u, persistedSettings1.size());

    //
    // Delete all settings
    //

    sm.DeletePersistedSettings(persistedSettings1[0].Key);
    sm.DeletePersistedSettings(persistedSettings1[1].Key);

    //
    // Verify nothing left
    //

    auto persistedSettings2 = sm.ListPersistedSettings();

    EXPECT_EQ(0u, persistedSettings2.size());
}

TEST(SettingsTests, BaseSettingsManager_E2E_LastModifiedSettings)
{
    auto testFileSystem = std::make_shared<TestFileSystem>();

    //
    // Set defaults
    //

    GlobalSettings.setting1 = 789.5f;
    GlobalSettings.setting2 = 242;
    GlobalSettings.setting3 = true;
    GlobalSettings.setting4 = "A Forest";
    GlobalSettings.setting5 = CustomValue("MyVal", 50);

    //
    // Create settings manager - defaults are taken here
    //

    TestSettingsManager sm(testFileSystem);

    //
    // Last-modified do not exist by default
    //

    EXPECT_FALSE(sm.HasLastModifiedSettingsPersisted());

    //
    // Change last-modified settings
    //

    GlobalSettings.setting2 = 243;
    GlobalSettings.setting5 = CustomValue("MyVal", 51);

    //
    // Save last-modified settings
    //

    sm.SaveLastModifiedSettings();

    //
    // Last-modified exist now
    //

    EXPECT_TRUE(sm.HasLastModifiedSettingsPersisted());

    //
    // Change settings again
    //

	GlobalSettings.setting1 = 0.0f;
	GlobalSettings.setting2 = 0;
	GlobalSettings.setting3 = false;
	GlobalSettings.setting4 = "";
	GlobalSettings.setting5 = CustomValue("", 0);

    GlobalSettingsImmediate.setting1 = 200.0f;
	GlobalSettingsImmediate.setting1 = 200;
    GlobalSettingsImmediate.setting3 = false;
	GlobalSettingsImmediate.setting4 = "";
	GlobalSettingsImmediate.setting5 = CustomValue("", 0);

    //
    // Load and enforce defaults and last-modified settings
    //

    auto ret = sm.EnforceDefaultsAndLastModifiedSettings();

	EXPECT_TRUE(ret);

    //
    // Verify enforced settings
    //

    EXPECT_EQ(0.0f, GlobalSettings.setting1);
    EXPECT_EQ(0u, GlobalSettings.setting2);
    EXPECT_EQ(false, GlobalSettings.setting3);
    EXPECT_EQ("", GlobalSettings.setting4);
    EXPECT_EQ("", GlobalSettings.setting5.Str);
    EXPECT_EQ(0, GlobalSettings.setting5.Int);

	EXPECT_EQ(789.5f, GlobalSettingsImmediate.setting1); // From defaults
	EXPECT_EQ(243u, GlobalSettingsImmediate.setting2); // Saved
	EXPECT_EQ(true, GlobalSettingsImmediate.setting3); // From defaults
	EXPECT_EQ("A Forest", GlobalSettingsImmediate.setting4); // From defaults
	EXPECT_EQ("MyVal", GlobalSettingsImmediate.setting5.Str); // Saved
	EXPECT_EQ(51, GlobalSettingsImmediate.setting5.Int); // Saved
}