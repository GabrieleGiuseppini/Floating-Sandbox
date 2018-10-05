#include <GameLib/ShaderManager.h>

#include <GameLib/GameException.h>

#include "gtest/gtest.h"

class ShaderManagerTests : public testing::Test
{
protected:

    std::map<std::string, std::string> MakeStaticParameters()
    {
        return std::map<std::string, std::string> {
            {"Z_DEPTH", "2.5678"},
            {"FOO", "BAR"}
        };
    }
};

TEST_F(ShaderManagerTests, SplitsShaders)
{
    std::string source = R"(
    ###VERTEX
vfoo
    ###FRAGMENT
 fbar
)";

    auto [vertexSource, fragmentSource] = ShaderManager::SplitSource(source);

    EXPECT_EQ("vfoo\n", vertexSource);
    EXPECT_EQ(" fbar\n", fragmentSource);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMissingVertexSection)
{
    std::string source = R"(
vfoo
###FRAGMENT
fbar
    )";

    EXPECT_THROW(
        ShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMissingVertexSection_EmptyFile)
{
    std::string source = "";

    EXPECT_THROW(
        ShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMissingFragmentSection)
{
    std::string source = R"(
###VERTEX
vfoo
fbar
    )";

    EXPECT_THROW(
        ShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_Single)
{
    std::string source = R"(

   FOO=56.8)";

    std::map<std::string, std::string> params;
    ShaderManager::ParseLocalStaticParameters(source, params);

    EXPECT_EQ(1, params.size());

    auto const & it = params.find("FOO");
    ASSERT_NE(it, params.end());
    EXPECT_EQ("56.8", it->second);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_Multiple)
{
    std::string source = R"(

FOO = 67
BAR = 89)";

    std::map<std::string, std::string> params;
    ShaderManager::ParseLocalStaticParameters(source, params);

    EXPECT_EQ(2, params.size());

    auto const & it1 = params.find("FOO");
    ASSERT_NE(it1, params.end());
    EXPECT_EQ("67", it1->second);

    auto const & it2 = params.find("BAR");
    ASSERT_NE(it2, params.end());
    EXPECT_EQ("89", it2->second);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_ErrorsOnRepeatedParameter)
{
    std::string source = R"(

FOO = 67
BAR = 89
FOO = 67
)";

    std::map<std::string, std::string> params;
    EXPECT_THROW(
        ShaderManager::ParseLocalStaticParameters(source, params),
        GameException);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_ErrorsOnMalformedDefinition)
{
    std::string source = R"(

FOO = 67
  g

)";

    std::map<std::string, std::string> params;
    EXPECT_THROW(
        ShaderManager::ParseLocalStaticParameters(source, params),
        GameException);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Single)
{
    std::string source = "hello world %Z_DEPTH% bar";

    auto result = ShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Multiple_Different)
{
    std::string source = "hello world %Z_DEPTH% bar\n foo %FOO% hello";

    auto result = ShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar\n foo BAR hello", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Multiple_Repeated)
{
    std::string source = "hello world %Z_DEPTH% bar\n foo %Z_DEPTH% hello";

    auto result = ShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar\n foo 2.5678 hello", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_ErrorsOnUnrecognizedParameter)
{
    std::string source = "a %Z_BAR% b";

    EXPECT_THROW(
        ShaderManager::SubstituteStaticParameters(source, MakeStaticParameters()),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsDynamicParameters_Single)
{
    std::string source = "  uniform float paramAmbientLightIntensity;\n";

    auto result = ShaderManager::ExtractDynamicParameters(source);

    ASSERT_EQ(1, result.size());
    EXPECT_EQ(1, result.count(ShaderManager::DynamicParameterType::AmbientLightIntensity));
}

TEST_F(ShaderManagerTests, ExtractsDynamicParameters_Multiple)
{
    std::string source = R"!!!(
uniform float paramAmbientLightIntensity;
foobar;
uniform mat4 paramOrthoMatrix;
)!!!";

    auto result = ShaderManager::ExtractDynamicParameters(source);

    ASSERT_EQ(2, result.size());
    EXPECT_EQ(1, result.count(ShaderManager::DynamicParameterType::AmbientLightIntensity));
    EXPECT_EQ(1, result.count(ShaderManager::DynamicParameterType::OrthoMatrix));
}

TEST_F(ShaderManagerTests, ExtractsDynamicParameters_ErrorsOnUnrecognizedParameter)
{
    std::string source = R"!!!(
uniform float paramAmbientLightIntensity;
foobar;
uniform mat4 paramOrthoMatriz;
)!!!";

    EXPECT_THROW(
        ShaderManager::ExtractDynamicParameters(source),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsDynamicParameters_ErrorsOnRedefinedParameter)
{
    std::string source = R"!!!(
uniform float paramAmbientLightIntensity;
foobar;
uniform mat4 paramAmbientLightIntensity;
)!!!";

    EXPECT_THROW(
        ShaderManager::ExtractDynamicParameters(source),
        GameException);
}