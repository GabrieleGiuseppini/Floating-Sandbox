#include <UILib/LayoutHelper.h>

#include "gmock/gmock.h"

class _MockLayoutHelperHandler
{
public:

    MOCK_METHOD2(OnBegin, void(int nCols, int nRows));
    MOCK_METHOD2(OnLayout, void(std::optional<int> element, IntegralCoordinates const & coords));

    _MockLayoutHelperHandler()
        : onBegin([this](int nCols, int nRows) { this->OnBegin(nCols, nRows); })
        , onLayout([this](std::optional<int> element, IntegralCoordinates const & coords) { this->OnLayout(element, coords); })
    {}

    std::function<void(int nCols, int nRows)> onBegin;
    std::function<void(std::optional<int> element, IntegralCoordinates const & coords)> onLayout;
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
    EXPECT_CALL(handler, OnLayout(_, _)).Times(0);

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
class UndecoratedOnlyLayoutTest : public testing::TestWithParam<std::tuple<size_t, int, int, int>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    LayoutHelperTests,
    UndecoratedOnlyLayoutTest,
    ::testing::Values(
        std::make_tuple(1, 1, 0, 1)
        , std::make_tuple(2, 3, -1, 1)
        , std::make_tuple(3, 3, -1, 1)
        , std::make_tuple(4, 5, -2, 1)
        , std::make_tuple(5, 5, -2, 1)
        , std::make_tuple(6, 7, -3, 1)
        , std::make_tuple(7, 7, -3, 1)
        , std::make_tuple(8, 9, -4, 1)
        , std::make_tuple(9, 9, -4, 1)
        , std::make_tuple(10, 11, -5, 1)
        , std::make_tuple(11, 11, -5, 1)
        , std::make_tuple(12, 11, -5, 2)
        , std::make_tuple(13, 11, -5, 2)
        , std::make_tuple(21, 11, -5, 2)
        , std::make_tuple(22, 11, -5, 2)
        , std::make_tuple(23, 13, -6, 2)
        , std::make_tuple(24, 13, -6, 2)
        , std::make_tuple(33, 17, -8, 2)
        , std::make_tuple(34, 17, -8, 2)
    ));
TEST_P(UndecoratedOnlyLayoutTest, UndecoratedOnlyLayoutTest)
{
    size_t const nElements = std::get<0>(GetParam());
    int const expectedNCols = std::get<1>(GetParam());
    int const expectedColStart = std::get<2>(GetParam());
    int const expectedNRows = std::get<3>(GetParam());

    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements;
    elements.reserve(nElements);
    for (size_t i = 0; i < nElements; ++i)
        elements.emplace_back(int(i), std::nullopt);

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(expectedNCols, expectedNRows)).Times(1);

    size_t iElement = 0;
    for (int row = 0; row < expectedNRows; ++row)
    {
        for (int col = expectedColStart, w = 0; w < expectedNCols; ++col, ++w)
        {
            if (iElement < nElements)
            {
                EXPECT_CALL(handler, OnLayout(std::optional<int>(int(iElement)), IntegralCoordinates(col, row))).Times(1);
                ++iElement;
            }
            else
                EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(col, row))).Times(1);
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

    EXPECT_EQ(iElement, nElements);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_Zero)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(0, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(1, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(0, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_MinusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(-1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_PlusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(1, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_MinusTwo)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(-2, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(5, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(-2, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(2, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_PlusTwo)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(2, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(5, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-2, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(2, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_MinusThree)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(-3, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(7, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(-3, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-2, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(2, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(3, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedOnlyLayout_One_PlusOnePlusOne)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 45, IntegralCoordinates(1, 1) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 2)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(-1, 1))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 1))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(45), IntegralCoordinates(1, 1))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedConflict_FirstSlot)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 1, IntegralCoordinates(-1, 0) },
        { 2, IntegralCoordinates(-1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(1), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(2), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedConflict_MiddleSlot)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 1, IntegralCoordinates(0, 0) },
        { 2, IntegralCoordinates(0, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(2), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(1), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(1, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

TEST(LayoutHelperTests, DecoratedConflict_LastSlot)
{
    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements{
        { 1, IntegralCoordinates(1, 0) },
        { 2, IntegralCoordinates(1, 0) }
    };

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(3, 1)).Times(1);

    EXPECT_CALL(handler, OnLayout(std::optional<int>(2), IntegralCoordinates(-1, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(std::nullopt), IntegralCoordinates(0, 0))).Times(1);
    EXPECT_CALL(handler, OnLayout(std::optional<int>(1), IntegralCoordinates(1, 0))).Times(1);

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        11,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}

// Decorated, Undecorated count (IDs will start from 1000), expected width, col start, expected height, expected opt<IDs>
class DecoratedAndUndecoratedLayoutTest : public testing::TestWithParam<
    std::tuple<
        std::vector<std::tuple<int, int, int>>,
        size_t,
        int, int, int,
        std::vector<std::optional<int>>>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    LayoutHelperTests,
    DecoratedAndUndecoratedLayoutTest,
    ::testing::Values(
        // [Undec][Dec][.]
        std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, 0, 0) },
            1,
            3, -1, 1,
            std::vector<std::optional<int>>{1000, 10, std::nullopt})
        // [Dec][Undec][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -1, 0) },
            1,
            3, -1, 1,
            std::vector<std::optional<int>>{10, 1000, std::nullopt})
        // [Undec][.][Dec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, 1, 0) },
            1,
            3, -1, 1,
            std::vector<std::optional<int>>{1000, std::nullopt, 10})

        // [Dec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -1, 0) },
            2,
            3, -1, 1,
            std::vector<std::optional<int>>{10, 1000, 1001})
        // [Undec][Dec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, 0, 0) },
            2,
            3, -1, 1,
            std::vector<std::optional<int>>{1000, 10, 1001})
        // [Undec][Undec][Dec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, 1, 0) },
            2,
            3, -1, 1,
            std::vector<std::optional<int>>{1000, 1001, 10})

        // [Undec][Dec][Undec][Undec][.]: grows cols only after having filled-in empty spaces, and grows by two
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -1, 0) },
            3,
            5, -2, 1,
            std::vector<std::optional<int>>{1000, 10, 1001, 1002, std::nullopt})

        // Right before MaxWidth, one dec
        // [Undec][Dec][Undec][Undec][Undec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0) },
            6,
            7, -3, 1,
            std::vector<std::optional<int>>{1000, 10, 1001, 1002, 1003, 1004, 1005, std::nullopt})

        // Right before MaxWidth, two dec's
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            5,
            7, -3, 1,
            std::vector<std::optional<int>>{1000, 10, 1001, 1002, 1003, 11, 1004, std::nullopt})

        // With one row, one more than MaxWidth makes it add the second row
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        // [Undec][.][.][.][.][.][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            6,
            7, -3, 2,
            std::vector<std::optional<int>>{
                1000, 10, 1001, 1002, 1003, 11, 1004
                , 1005, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt})

        // With one row, one more than MaxWidth makes it add the second row
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        // [Undec][Undec][.][.][.][.][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            7,
            7, -3, 2,
            std::vector<std::optional<int>>{
                1000, 10, 1001, 1002, 1003, 11, 1004
                , 1005, 1006, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt})

        // With one row, one more than MaxWidth makes it add the second row
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            12,
            7, -3, 2,
            std::vector<std::optional<int>>{
                1000, 10, 1001, 1002, 1003, 11, 1004
                , 1005, 1006, 1007, 1008, 1009, 1010, 1011})

        // With two rows, one more adds two full columns
        // [Undec][Undec][Dec][Undec][Undec][Undec][Dec][Undec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][.][.][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            13,
            9, -4, 2,
            std::vector<std::optional<int>>{
                1000, 1001, 10, 1002, 1003, 1004, 11, 1005, 1006
                , 1007, 1008, 1009, 1010, 1011, 1012, std::nullopt, std::nullopt, std::nullopt })

        // With two rows, one more adds two full columns
        // [Undec][Undec][Dec][Undec][Undec][Undec][Dec][Undec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][Undec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{ std::make_tuple(10, -2, 0), std::make_tuple(11, 2, 0) },
            16,
            9, -4, 2,
            std::vector<std::optional<int>>{
                1000, 1001, 10, 1002, 1003, 1004, 11, 1005, 1006
                , 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015 })

        // Starts third row only when something's already there
        // [Dec][Undec][Undec][Undec][Dec]
        // [Undec][Undec][Undec][Undec][Undec]
        // [Undec][Undec][Dec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{
                    std::make_tuple(10, -2, 0)
                    , std::make_tuple(11, 2, 0)
                    , std::make_tuple(12, 0, 2)},
            12,
            5, -2, 3,
            std::vector<std::optional<int>>{
                10, 1000, 1001, 1002, 11
                , 1003, 1004, 1005, 1006, 1007
                , 1008, 1009, 12, 1010, 1011 })

        // After third row, grows evenly on both sides
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][Undec]
        // [Undec][.][.][Dec][.][.][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{
                    std::make_tuple(10, -2, 0)
                    , std::make_tuple(11, 2, 0)
                    , std::make_tuple(12, 0, 2)},
            13,
            7, -3, 3,
            std::vector<std::optional<int>>{
                1000, 10, 1001, 1002, 1003, 11, 1004
                , 1005, 1006, 1007, 1008, 1009, 1010, 1011
                , 1012, std::nullopt, std::nullopt, 12, std::nullopt, std::nullopt, std::nullopt })

        // After third row, grows evenly on both sides
        // [Undec][Dec][Undec][Undec][Undec][Dec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][Undec]
        // [Undec][Undec][Undec][Dec][Undec][Undec][Undec]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{
                    std::make_tuple(10, -2, 0)
                    , std::make_tuple(11, 2, 0)
                    , std::make_tuple(12, 0, 2)},
            18, // Fill
            7, -3, 3,
            std::vector<std::optional<int>>{
                1000, 10, 1001, 1002, 1003, 11, 1004
                , 1005, 1006, 1007, 1008, 1009, 1010, 1011
                , 1012, 1013, 1014, 12, 1015, 1016, 1017 })

        // After third row, grows evenly on both sides
        // [Undec][Undec][Dec][Undec][Undec][Undec][Dec][Undec][Undec]
        // [Undec][Undec][Undec][Undec][Undec][Undec][Undec][Undec][Undec]
        // [Undec][Undec][Undec][.][Dec][.][.][.][.]
        , std::make_tuple(
            std::vector<std::tuple<int, int, int>>{
                    std::make_tuple(10, -2, 0)
                    , std::make_tuple(11, 2, 0)
                    , std::make_tuple(12, 0, 2)},
            19, // One more
            9, -4, 3,
            std::vector<std::optional<int>>{
                1000, 1001, 10, 1002, 1003, 1004, 11, 1005, 1006
                , 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015
                , 1016, 1017, 1018, std::nullopt, 12, std::nullopt, std::nullopt, std::nullopt, std::nullopt })

    ));
TEST_P(DecoratedAndUndecoratedLayoutTest, DecoratedAndUndecoratedLayoutTest)
{
    std::vector<std::tuple<int, int, int>> const & decoratedElements = std::get<0>(GetParam());
    size_t nUndecoratedElements = std::get<1>(GetParam());
    int const expectedWidth = std::get<2>(GetParam());
    int const expectedColStart = std::get<3>(GetParam());
    int const expectedHeight = std::get<4>(GetParam());

    std::vector<std::optional<int>> const & expectedIds = std::get<5>(GetParam());

    // Prepare data

    std::vector<LayoutHelper::LayoutElement<int>> elements;

    for (auto const & decoratedElement : decoratedElements)
    {
        elements.emplace_back(
            std::get<0>(decoratedElement),
            IntegralCoordinates(std::get<1>(decoratedElement), std::get<2>(decoratedElement)));
    }

    for (size_t i = 0; i < nUndecoratedElements; ++i)
    {
        elements.emplace_back(
            int(1000 + i),
            std::nullopt);
    }

    // Setup expectations

    MockHandler handler;

    InSequence s;

    EXPECT_CALL(handler, OnBegin(expectedWidth, expectedHeight)).Times(1);

    size_t iElement = 0;
    for (int row = 0; row < expectedHeight; ++row)
    {
        for (int col = expectedColStart, w = 0; w < expectedWidth; ++col, ++w)
        {
            EXPECT_LT(iElement, expectedIds.size());
            EXPECT_CALL(handler, OnLayout(expectedIds[iElement], IntegralCoordinates(col, row))).Times(1);
            ++iElement;
        }
    }

    // Layout

    LayoutHelper::Layout<int>(
        elements,
        7,
        handler.onBegin,
        handler.onLayout);

    // Verify

    Mock::VerifyAndClear(&handler);
}