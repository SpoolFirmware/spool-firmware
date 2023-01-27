#include "fix16.h"
#include "munit.h"

static void test_copy_sign(void)
{
    fix16_t b = F16(25.12);
    fix16_t c = F16(10000.5);
    fix16_t c_neg = F16(-10000.5);
    fix16_t d = fix16_copy_sign(b, c_neg);
    munit_assert_int32(d, ==, -b);
    fix16_t c_1 = fix16_copy_sign(c, c_neg);
    munit_assert_int32(c_1, ==, c_neg);
}

int main() {
    fix16_t a = F16(1.5);
    fix16_t b = F16(2.5);
    fix16_t sum = fix16_add(a, b);
    munit_assert_int32(4, ==, fix16_to_int(sum));
    test_copy_sign();
    return 0;
}