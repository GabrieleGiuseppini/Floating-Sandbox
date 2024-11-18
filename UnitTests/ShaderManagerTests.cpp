#include <Game/RenderTypes.h>
#include <Game/ShaderTypes.h>

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameException.h>

#include "gtest/gtest.h"

using TestShaderManager = ShaderManager<Render::ShaderManagerTraits>;

class ShaderManagerTests : public testing::Test
{
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

TEST_F(ShaderManagerTests, ProcessesIncludes_AllowsLoops)
{
    std::string source = R"(
aaa
#include "inc1.glsl"
bbb
)";

    std::unordered_map<std::string, std::pair<bool, std::string>> includeFiles;
    includeFiles["inc2.glslinc"] = std::make_pair<bool, std::string>(true, std::string("#include \"inc1.glsl\"\n"));
    includeFiles["inc1.glsl"] = { false, std::string("sancho\n#include \"inc2.glslinc\"") };

    auto resolvedSource = TestShaderManager::ResolveIncludes(
        source,
        includeFiles);

    EXPECT_EQ("\naaa\nsancho\n\nbbb\n", resolvedSource);
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
    std::string source = R"(###VERTEX-120
vfoo
    ###FRAGMENT-999
 fbar
)";

    auto [vertexSource, fragmentSource] = TestShaderManager::SplitSource(source);

    EXPECT_EQ("#version 120\nvfoo\n", vertexSource);
    EXPECT_EQ("#version 999\n fbar\n", fragmentSource);
}

TEST_F(ShaderManagerTests, SplitsShaders_DuplicatesCommonSectionToVertexAndFragment)
{
    std::string source = R"(  #define foo bar this is common

another define
    ###VERTEX-120
vfoo
    ###FRAGMENT-120
 fbar
)";

    auto [vertexSource, fragmentSource] = TestShaderManager::SplitSource(source);

    EXPECT_EQ("#version 120\n  #define foo bar this is common\n\nanother define\nvfoo\n", vertexSource);
    EXPECT_EQ("#version 120\n  #define foo bar this is common\n\nanother define\n fbar\n", fragmentSource);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMalformedVertexSection)
{
    std::string source = R"(###VERTEX-1a0
vfoo
    ###FRAGMENT-999
 fbar
)";

    EXPECT_THROW(
        TestShaderManager::SplitSource(source),
        GameException);
}

TEST_F(ShaderManagerTests, SplitsShaders_ErrorsOnMissingVertexSection)
{
    std::string source = R"(vfoo
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
    std::string source = R"(###VERTEX
vfoo
fbar
    )";

    EXPECT_THROW(
        TestShaderManager::SplitSource(source),
        GameException);
}
