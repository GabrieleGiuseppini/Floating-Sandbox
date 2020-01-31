#include <GameCore/Vectors.h>

#include <GameCore/SysSpecifics.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(VectorsTests, Sum_2f)
{
	vec2f a(1.0f, 5.0f);
	vec2f b(2.0f, 4.0f);
	vec2f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
	EXPECT_EQ(c.y, 9.0f);
}

TEST(VectorsTests, Sum_4f)
{
    vec4f a(1.0f, 5.0f, 20.0f, 100.4f);
    vec4f b(2.0f, 4.0f, 40.0f, 200.0f);
    vec4f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 9.0f);
    EXPECT_EQ(c.z, 60.0f);
    EXPECT_EQ(c.w, 300.4f);
}

class Normalization2fTest : public testing::TestWithParam<std::tuple<vec2f, vec2f>>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    VectorTests,
    Normalization2fTest,
    ::testing::Values(
        std::make_tuple(vec2f{ 1.0f, 0.0f }, vec2f{ 1.0f, 0.0f }),
        std::make_tuple(vec2f{ 1.0f, 1.0f }, vec2f{ 1.0f / sqrt(2.0f), 1.0f / sqrt(2.0f) }),
        std::make_tuple(vec2f{ 3.0f, 4.0f }, vec2f{ 3.0f / 5.0f, 4.0f / 5.0f}),
        std::make_tuple(vec2f{ 0.0f, 0.0f }, vec2f{ 0.0f, 0.0f })
    ));

// TODOTEST
inline vec2f normalise_SSEx1(vec2f const & v) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 x = _mm_load_ss(&(v.x));
    __m128 y = _mm_load_ss(&(v.y));

    __m128 len = _mm_sqrt_ss(
        _mm_add_ss(
            _mm_mul_ss(x, x),
            _mm_mul_ss(y, y)));

    __m128 invLen = _mm_div_ss(One, len);
    __m128 validMask = _mm_cmpneq_ss(invLen, Zero);
    invLen = _mm_and_ps(invLen, validMask);

    x = _mm_mul_ss(x, invLen);
    y = _mm_mul_ss(y, invLen);

    return vec2f(_mm_cvtss_f32(x), _mm_cvtss_f32(y));
}

inline vec2f normalise_SSEx1(vec2f const & v, float length) noexcept
{
    __m128 const Zero = _mm_setzero_ps();
    __m128 const One = _mm_set_ss(1.0f);

    __m128 _l = _mm_set_ss(length);
    __m128 _revl = _mm_div_ss(One, _l);
    __m128 validMask = _mm_cmpneq_ss(_l, Zero);
    _revl = _mm_and_ps(_revl, validMask);

    __m128 _x = _mm_mul_ss(_mm_load_ss(&(v.x)), _revl);
    __m128 _y = _mm_mul_ss(_mm_load_ss(&(v.y)), _revl);

    return vec2f(_mm_cvtss_f32(_x), _mm_cvtss_f32(_y));
}

TEST_P(Normalization2fTest, Normalization2fTest)
{
    vec2f input = std::get<0>(GetParam());
    vec2f expected = std::get<1>(GetParam());

    // TODO
    //vec2f calcd1 = input.normalise();
    vec2f calcd1 = normalise_SSEx1(input);
    EXPECT_TRUE(ApproxEquals(calcd1.x, expected.x, 0.0001f));
    EXPECT_TRUE(ApproxEquals(calcd1.y, expected.y, 0.0001f));

    float len = input.length();
    // TODO
    //vec2f calcd2 = input.normalise(len);
    vec2f calcd2 = normalise_SSEx1(input, len);
    EXPECT_TRUE(ApproxEquals(calcd2.x, expected.x, 0.0001f));
    EXPECT_TRUE(ApproxEquals(calcd2.y, expected.y, 0.0001f));
}