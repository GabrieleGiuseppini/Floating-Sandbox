#include <UIControls/LayoutHelper.h>

#include "gmock/gmock.h"

class _MockLayoutHelperHandler
{
public:

    MOCK_METHOD2(OnBegin, void(int width, int height));
    MOCK_METHOD3(OnLayout, void(std::optional<int> element, int x, int y));

    _MockLayoutHelperHandler()
        : onBegin([this](int width, int height) { this->OnBegin(width, height); })
        , onLayout([this](std::optional<int> element, int x, int y) { this->OnLayout(element, x, y); })
    {}

    std::function<void(int width, int height)> onBegin;
    std::function<void(std::optional<int> element, int x, int y)> onLayout;
};

using namespace ::testing;

using MockHandler = StrictMock<_MockLayoutHelperHandler>;

TEST(LayoutHelperTests, Empty)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
    };

    // Setup expectations

    MockHandler handler;

    EXPECT_CALL(handler, OnBegin(0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(_, _, _)).Times(0);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

// NElements, expected width, col start, expected height
class NonPanelLayoutTest : public testing::TestWithParam<std::tuple<size_t, int, int, int>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    LayoutHelperTests,
    NonPanelLayoutTest,
    ::testing::Values(
        std::make_tuple(1, 1, 0, 1)
        , std::make_tuple(2, 2, -1, 1)
        , std::make_tuple(3, 3, -1, 1)
        , std::make_tuple(6, 6, -3, 1)
        , std::make_tuple(7, 7, -3, 1)
        , std::make_tuple(10, 10, -5, 1)
        , std::make_tuple(11, 11, -5, 1)
        , std::make_tuple(12, 11, -5, 2)
        , std::make_tuple(13, 11, -5, 2)
        , std::make_tuple(21, 11, -5, 2)
        , std::make_tuple(22, 11, -5, 2)
        , std::make_tuple(23, 11, -5, 3)
        , std::make_tuple(24, 11, -5, 3)
        , std::make_tuple(33, 11, -5, 3)
        , std::make_tuple(34, 11, -5, 4)
    ));
TEST_P(NonPanelLayoutTest, NonPanelLayoutTest)
{
    size_t const nElements = std::get<0>(GetParam());
    int const expectedWidth = std::get<1>(GetParam());
    int const expectedColStart = std::get<2>(GetParam());
    int const expectedHeight = std::get<3>(GetParam());

    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements;
    elements.reserve(nElements);
    for (size_t i = 0; i < nElements; ++i)
        elements.emplace_back(int(i), std::nullopt);

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(expectedWidth, expectedHeight)).Times(1);

    size_t iElement = 0;
    for (int row = 0; row < expectedHeight; ++row)
    {
        for (int col = expectedColStart, w = 0; w < expectedWidth; ++col, ++w)
        {
            if (iElement < nElements)
                EXPECT_CALL(handler, OnLayout(std::optional<int>(int(iElement)), col, row)).Times(1);
            else
                EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), col, row)).Times(1);

            ++iElement;
        }
    }

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_Zero)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(0, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(1, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), 0, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_MinusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(-1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 1, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_PlusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), 1, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_MinusTwo)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(-2, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(5, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), -2, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 2, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_PlusTwo)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(2, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(5, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -2, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), 2, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_MinusThree)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(-3, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(7, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), -3, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -2, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 2, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 3, 0)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, WithPanelLayout_One_PlusOnePlusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, std::make_pair<int, int>(1, 1) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 2)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 0)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 1, 0)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), -1, 1)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), 0, 1)).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), 1, 1)).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

// NonPanelAndPanel TODO