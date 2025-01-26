#include <Core/Utils.h>

#include "gtest/gtest.h"

class LTrimTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    LTrimTests,
    LTrimTest,
    ::testing::Values( // Input, Output
        std::make_tuple("foo", "foo"),
        std::make_tuple(" foo", "foo"),
        std::make_tuple("foo ", "foo "),
        std::make_tuple(" foo ", "foo "),
        std::make_tuple("  foo  ", "foo  "),
        std::make_tuple("", ""),
        std::make_tuple("  ", "")
    )
);

TEST_P(LTrimTest, BasicCases)
{
    auto const result = Utils::LTrim(std::get<0>(GetParam()));
    auto const expected = std::get<1>(GetParam());

    EXPECT_EQ(result, expected);
}

class RTrimTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    RTrimTests,
    RTrimTest,
    ::testing::Values( // Input, Output
        std::make_tuple("foo", "foo"),
        std::make_tuple(" foo", " foo"),
        std::make_tuple("foo ", "foo"),
        std::make_tuple(" foo ", " foo"),
        std::make_tuple("  foo  ", "  foo"),
        std::make_tuple("", ""),
        std::make_tuple("  ", "")
    )
);

TEST_P(RTrimTest, BasicCases)
{
    auto const result = Utils::RTrim(std::get<0>(GetParam()));
    auto const expected = std::get<1>(GetParam());

    EXPECT_EQ(result, expected);
}

class TrimTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    TrimTests,
    TrimTest,
    ::testing::Values( // Input, Output
        std::make_tuple("foo", "foo"),
        std::make_tuple(" foo", "foo"),
        std::make_tuple("foo ", "foo"),
        std::make_tuple(" foo ", "foo"),
        std::make_tuple("  foo  ", "foo"),
        std::make_tuple("", ""),
        std::make_tuple("  ", "")
    )
);

TEST_P(TrimTest, BasicCases)
{
    auto const result = Utils::Trim(std::get<0>(GetParam()));
    auto const expected = std::get<1>(GetParam());

    EXPECT_EQ(result, expected);
}

TEST(ChangelistToHtml, Basic)
{
    std::string inputStr = R"(
    - Line 1
    - Line 2)";

    auto input = std::stringstream(inputStr);

    std::string const output = Utils::ChangelistToHtml(input);

    EXPECT_EQ(output, "<ul><ul><li>Line 1</li><li>Line 2</li></ul></ul>");
}

class ChangelistToHtmlTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    ChangelistToHtmlTests,
    ChangelistToHtmlTest,
    ::testing::Values( // Input, Output
        std::make_tuple("", ""),
        std::make_tuple("  ", ""),

        // Two level-one, new-line at and
        std::make_tuple(R"(
- Line 1
- Line 2
)", "<ul><li>Line 1</li><li>Line 2</li></ul>"),

        // Two level-two, no new-line at end
        std::make_tuple(R"(
    - Line 1
    - Line 2)", "<ul><ul><li>Line 1</li><li>Line 2</li></ul></ul>"),

        // Two level-three, no new-line at end
        std::make_tuple(R"(
        - Line 1
        - Line 2)", "<ul><ul><ul><li>Line 1</li><li>Line 2</li></ul></ul></ul>"),

        // Two level-two, mixed chars
        std::make_tuple("\t- Line 1\n    - Line 2", "<ul><ul><li>Line 1</li><li>Line 2</li></ul></ul>"),

        // Two level-three, mixed chars
        std::make_tuple("\t    - Line 1\n    \t- Line 2", "<ul><ul><ul><li>Line 1</li><li>Line 2</li></ul></ul></ul>"),

        // Multiple levels (real-world)
        std::make_tuple(R"(- Line 1
- Line 2
    - Line 3
    - Line 4
- Line 5
    - Line 6
        - Line 7
)", "<ul><li>Line 1</li><li>Line 2</li><ul><li>Line 3</li><li>Line 4</li></ul><li>Line 5</li><ul><li>Line 6</li><ul><li>Line 7</li></ul></ul></ul>"),

        // Line without bullet
        std::make_tuple(R"(
- Line 1
   Line 2
)", "<ul><li>Line 1<br/>Line 2</li></ul>")

    )
);

TEST_P(ChangelistToHtmlTest, Cases)
{
    auto input = std::stringstream(std::get<0>(GetParam()));
    auto const result = Utils::ChangelistToHtml(input);
    auto const expected = std::get<1>(GetParam());

    EXPECT_EQ(result, expected);
}
