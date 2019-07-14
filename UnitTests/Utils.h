#define SIMDPP_ARCH_X86_SSSE3
#include "simdpp/simd.h"

void DoSomething(simdpp::float32<4> const & v);

#include "gtest/gtest.h"

::testing::AssertionResult ApproxEquals(float a, float b, float tolerance);
