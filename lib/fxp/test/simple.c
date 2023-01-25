#include "fix16.h"
#include "munit.h"

int main() {
    fix16_t a = F16(1.5);
    fix16_t b = F16(2.5);
    fix16_t sum = fix16_add(a, b);
    munit_assert_int32(4, ==, fix16_to_int(sum));
    return 0;
}