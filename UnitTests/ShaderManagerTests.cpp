#include <Game/RenderTypes.h>
#include <Game/ShaderTypes.h>

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameException.h>

#include "gtest/gtest.h"

using TestShaderManager = ShaderManager<Render::ShaderManagerTraits>;

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

TEST_F(ShaderManagerTests, ProcessesIncludes_OneLevel)
{
    std::string source = R"(
aaa
  #include "inc1.glsl"
bbb
)";

    std::unordered_map<std::string, std::pair<bool, std::string>> includeFiles;
    includeFiles["ggg.glsl"] = std::make_pair<bool, std::string>(true, std::string("   \n zorro \n"));
    includeFiles["inc1.glsl"] = { false, std::string(" \n sancho \n") };

    auto resolvedSource = TestShaderManager::ResolveIncludes(
        source,
        includeFiles);

    EXPECT_EQ("\naaa\n \n sancho \n\nbbb\n", resolvedSource);
}

TEST_F(ShaderManagerTests, ProcessesIncludes_MultipleLevels)
{
    std::string source = R"(
aaa
  #include "inc1.glsl"
bbb
)";

    std::unordered_map<std::string, std::pair<bool, std::string>> includeFiles;
    includeFiles["inc2.glslinc"] = std::make_pair<bool, std::string>(true, std::string("nano\n"));
    includeFiles["inc1.glsl"] = { false, std::string("sancho\n#include \"inc2.glslinc\"") };

    auto resolvedSource = TestShaderManager::ResolveIncludes(
        source,
        includeFiles);

    EXPECT_EQ("\naaa\nsancho\nnano\n\nbbb\n", resolvedSource);
}

TEST_F(ShaderManagerTests, ProcessesIncludes_DetectsLoops)
{
    std::string source = R"(
aaa
#include "inc1.glsl"
bbb
)";

    std::unordered_map<std::string, std::pair<bool, std::string>> includeFiles;
    includeFiles["inc2.glslinc"] = std::make_pair<bool, std::string>(true, std::string("#include \"inc1.glsl\"\n"));
    includeFiles["inc1.glsl"] = { false, std::string("sancho\n#include \"inc2.glslinc\"") };

    EXPECT_THROW(
        TestShaderManager::ResolveIncludes(
            source,
            includeFiles),
        GameException);
}

TEST_F(ShaderManagerTests, ProcessesIncludes_ComplainsWhenIncludeNotFound)
{
    std::string source = R"(
aaa
  #include "inc1.glslinc"
bbb
)";

    std::unordered_map<std::string, std::pair<bool, std::string>> includeFiles;
    includeFiles["inc3.glslinc"] = std::make_pair<bool, std::string>(true, std::string("nano\n"));

    EXPECT_THROW(
        TestShaderManager::ResolveIncludes(
            source,
            includeFiles),
        GameException);
}

TEST_F(ShaderManagerTests, SplitsShaders)
{
    std::string source = R"(
    ###VERTEX
vfoo
    ###FRAGMENT
 fbar
)";

    auto [vertexSource, fragmentSource] = TestShaderManager::SplitSource(source);

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
        TestShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMissingVertexSection_EmptyFile)
{
    std::string source = "";

    EXPECT_THROW(
        TestShaderManager::SplitSource(source),
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
        TestShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_Single)
{
    std::string source = R"(

   FOO=56.8)";

    std::map<std::string, std::string> params;
    TestShaderManager::ParseLocalStaticParameters(source, params);

    EXPECT_EQ(1u, params.size());

    auto const & it = params.find("FOO");
    ASSERT_NE(it, params.end());
    EXPECT_EQ("56.8", it->second);
}

TEST_F(ShaderManagerTests, ParsesStaticParameters_Multiple)
{
    std::string source = R"(

FOO = 67, 87, 88

BAR = 89)";

    std::map<std::string, std::string> params;
    TestShaderManager::ParseLocalStaticParameters(source, params);

    EXPECT_EQ(2u, params.size());

    auto const & it1 = params.find("FOO");
    ASSERT_NE(it1, params.end());
    EXPECT_EQ("67, 87, 88", it1->second);

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
        TestShaderManager::ParseLocalStaticParameters(source, params),
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
        TestShaderManager::ParseLocalStaticParameters(source, params),
        GameException);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Single)
{
    std::string source = "hello world %Z_DEPTH% bar";

    auto result = TestShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Multiple_Different)
{
    std::string source = "hello world %Z_DEPTH% bar\n foo %FOO% hello";

    auto result = TestShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar\n foo BAR hello", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_Multiple_Repeated)
{
    std::string source = "hello world %Z_DEPTH% bar\n foo %Z_DEPTH% hello";

    auto result = TestShaderManager::SubstituteStaticParameters(source, MakeStaticParameters());

    EXPECT_EQ("hello world 2.5678 bar\n foo 2.5678 hello", result);
}

TEST_F(ShaderManagerTests, SubstitutesStaticParameters_ErrorsOnUnrecognizedParameter)
{
    std::string source = "a %Z_BAR% b";

    EXPECT_THROW(
        TestShaderManager::SubstituteStaticParameters(source, MakeStaticParameters()),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_Single)
{
    std::string source = "  uniform float paramEffectiveAmbientLightIntensity;\n";

    auto result = TestShaderManager::ExtractShaderParameters(source);

    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::EffectiveAmbientLightIntensity));
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_Multiple)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
foobar;
uniform mat4 paramOrthoMatrix;
)!!!";

    auto result = TestShaderManager::ExtractShaderParameters(source);

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::EffectiveAmbientLightIntensity));
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::OrthoMatrix));
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_IgnoresTrailingComment)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity; // this is a comment
foobar;
uniform mat4 paramOrthoMatrix;
)!!!";

    auto result = TestShaderManager::ExtractShaderParameters(source);

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::EffectiveAmbientLightIntensity));
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::OrthoMatrix));
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_IgnoresCommentedOutParameters)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
foobar;
//uniform mat4 paramOrthoMatrix;
)!!!";

    auto result = TestShaderManager::ExtractShaderParameters(source);

    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(1u, result.count(Render::ProgramParameterType::EffectiveAmbientLightIntensity));
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_ErrorsOnUnrecognizedParameter)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
foobar;
uniform mat4 paramOrthoMatriz;
)!!!";

    EXPECT_THROW(
        TestShaderManager::ExtractShaderParameters(source),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsShaderParameters_ErrorsOnRedefinedParameter)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
foobar;
uniform mat4 paramEffectiveAmbientLightIntensity;
)!!!";

    EXPECT_THROW(
        TestShaderManager::ExtractShaderParameters(source),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsVertexAttributeNames_Single)
{
    std::string source = "  in vec4 inShipPointColor;\n";

    auto result = TestShaderManager::ExtractVertexAttributeNames(source);

    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(1u, result.count("ShipPointColor"));
}

TEST_F(ShaderManagerTests, ExtractsVertexAttributeNames_Multiple)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
in matfoo lopo lopo inShipPointColor;
foobar;
in mat4 inGenericMipMappedTexture3;
)!!!";

    auto result = TestShaderManager::ExtractVertexAttributeNames(source);

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ(1u, result.count("ShipPointColor"));
    EXPECT_EQ(1u, result.count("GenericMipMappedTexture3"));
}

TEST_F(ShaderManagerTests, ExtractsVertexAttributeNames_ErrorsOnUnrecognizedAttribute)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
foobar;
in mat4 inHeadPosition;
)!!!";

    EXPECT_THROW(
        TestShaderManager::ExtractVertexAttributeNames(source),
        GameException);
}

TEST_F(ShaderManagerTests, ExtractsVertexAttributeNames_ErrorsOnRedeclaredAttribute)
{
    std::string source = R"!!!(
uniform float paramEffectiveAmbientLightIntensity;
in mat4 inShipPointColor;
foobar;
in mat4 inShipPointColor;
)!!!";

    EXPECT_THROW(
        TestShaderManager::ExtractVertexAttributeNames(source),
        GameException);
}