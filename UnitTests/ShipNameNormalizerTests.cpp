#include <ShipBuilderLib/ShipNameNormalizer.h>

#include "gtest/gtest.h"

class ShipNameNormalizer_NormalizePrefixTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    ShipNameNormalizer_NormalizePrefixTest_Tests,
    ShipNameNormalizer_NormalizePrefixTest,
    ::testing::Values(
        // Idempotent
        std::make_tuple("R.M.S. Titanic", "R.M.S. Titanic"),

        // Idempotent with trimming
        std::make_tuple(" R.M.S. Titanic ", "R.M.S. Titanic"),
        std::make_tuple("    R.M.S. Titanic   ", "R.M.S. Titanic"),

        // Prefix stemming - with rest

        std::make_tuple("RMS Titanic", "R.M.S. Titanic"),
        std::make_tuple("RMS. Titanic", "R.M.S. Titanic"),
        std::make_tuple("RM.S Titanic", "R.M.S. Titanic"),
        std::make_tuple("R.MS Titanic", "R.M.S. Titanic"),
        std::make_tuple("R. MS Titanic", "R.M.S. Titanic"),
        std::make_tuple("   R.M.S. Titanic", "R.M.S. Titanic"),
        std::make_tuple("   RMS Titanic", "R.M.S. Titanic"),

        std::make_tuple("R.Smg. Titanic", "R.Smg. Titanic"),
        std::make_tuple("R.SMG. Titanic", "R.Smg. Titanic"),
        std::make_tuple("RSMG Titanic", "R.Smg. Titanic"),

        std::make_tuple("Tr.S.M.V. Titanic", "Tr.S.M.V. Titanic"),
        std::make_tuple("Tr SMV Titanic", "Tr.S.M.V. Titanic"),
        std::make_tuple("Tr SMV Titanic", "Tr.S.M.V. Titanic"),
        std::make_tuple(" Tr.SMV Titanic", "Tr.S.M.V. Titanic"),

        // Prefix stemming - without rest
        std::make_tuple("RMS", "R.M.S."),
        std::make_tuple(" RMS", "R.M.S."),
        std::make_tuple("RMS ", "R.M.S."),
        std::make_tuple("  RMS  ", "R.M.S."),

        // Longest
        std::make_tuple("A.B.C.D. Titanic", "A.B.C.D. Titanic"),
        std::make_tuple("A B C D Titanic", "A.B.C.D. Titanic"),

        // Unmatched
        std::make_tuple("RMSTitanic", "RMSTitanic"),

        // Empty string
        std::make_tuple("", ""),
        std::make_tuple(" ", ""),
        std::make_tuple("     ", "")
    ));

TEST_P(ShipNameNormalizer_NormalizePrefixTest, PrefixTests)
{
    ShipBuilder::ShipNameNormalizer normalizer(
        {
            "R.M.S.",
            "R.Smg.",
            "Tr.S.M.V.",
            "A.B.",
            "A.B.C.",
            "A.B.C.D."
        });

    std::string const & source = std::get<0>(GetParam());
    std::string const & expected = std::get<1>(GetParam());

    std::string actual = normalizer.NormalizeName(source);

    EXPECT_EQ(actual, expected);
}

class ShipNameNormalizer_NormalizeYearTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    ShipNameNormalizer_NormalizeYearTest_Tests,
    ShipNameNormalizer_NormalizeYearTest,
    ::testing::Values(
        // Idempotent
        std::make_tuple("Titanic (1912)", "Titanic (1912)"),

        // Idempotent with trimming
        std::make_tuple("Titanic (1912) ", "Titanic (1912)"),
        std::make_tuple("Titanic   (1912)  ", "Titanic (1912)"),

        // Year fixing

        std::make_tuple("Titanic 1912", "Titanic (1912)"),
        std::make_tuple("Titanic 1912 ", "Titanic (1912)"),
        std::make_tuple("Titanic 1912  ", "Titanic (1912)"),

        std::make_tuple("Titanic - 1912", "Titanic (1912)"),
        std::make_tuple("Titanic - 1912 ", "Titanic (1912)"),
        std::make_tuple("Titanic - 1912  ", "Titanic (1912)"),
        std::make_tuple("Titanic  -  1912  ", "Titanic (1912)"),

        std::make_tuple("Titanic ( 1912 )", "Titanic (1912)"),
        std::make_tuple("Titanic ( 1912 ) ", "Titanic (1912)"),
        std::make_tuple("Titanic ( 1912 )  ", "Titanic (1912)"),
        std::make_tuple("Titanic  ( 1912 )  ", "Titanic (1912)"),

        std::make_tuple("Titanic (   1912   )", "Titanic (1912)"),
        std::make_tuple("Titanic (   1912   ) ", "Titanic (1912)"),

        std::make_tuple("Titanic 1912 ", "Titanic (1912)"),
        std::make_tuple("Titanic   1912   ", "Titanic (1912)"),

        // Unmatched
        std::make_tuple("Titanic 191", "Titanic 191"),
        std::make_tuple("Titanic 19123", "Titanic 19123"),
        std::make_tuple("Titanic 191g", "Titanic 191g")
    ));

TEST_P(ShipNameNormalizer_NormalizeYearTest, YearTests)
{
    ShipBuilder::ShipNameNormalizer normalizer(
        {
            "R.M.S.",
            "R.Smg.",
            "Tr.S.M.V."
        });

    std::string const & source = std::get<0>(GetParam());
    std::string const & expected = std::get<1>(GetParam());

    std::string actual = normalizer.NormalizeName(source);

    EXPECT_EQ(actual, expected);
}

class ShipNameNormalizer_NormalizePrefixAndYearTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    ShipNameNormalizer_NormalizePrefixAndYearTests,
    ShipNameNormalizer_NormalizePrefixAndYearTest,
    ::testing::Values(
        // Idempotent
        std::make_tuple("R.M.S. Titanic (1912)", "R.M.S. Titanic (1912)"),

        // Both prefix and year
        std::make_tuple("RMS Titanic 1912", "R.M.S. Titanic (1912)"),
        std::make_tuple("R MS Titanic - 1912", "R.M.S. Titanic (1912)"),
        std::make_tuple("R-M-S Titanic( 1912 )", "R.M.S. Titanic (1912)"),

        // Space normalization
        std::make_tuple(" R.M.S.  Titanic   (1912)   ", "R.M.S. Titanic (1912)")
    ));

TEST_P(ShipNameNormalizer_NormalizePrefixAndYearTest, YearTests)
{
    ShipBuilder::ShipNameNormalizer normalizer(
        {
            "R.M.S.",
            "R.Smg.",
            "Tr.S.M.V."
        });

    std::string const & source = std::get<0>(GetParam());
    std::string const & expected = std::get<1>(GetParam());

    std::string actual = normalizer.NormalizeName(source);

    EXPECT_EQ(actual, expected);
}