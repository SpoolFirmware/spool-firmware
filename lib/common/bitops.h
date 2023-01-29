#pragma once

/* undefined for 0 */
inline unsigned int count_leading_zero(unsigned int num)
{
    return (unsigned int)__builtin_clz(num);
}

/* undefined for 0 */
inline unsigned int count_trailing_zero(unsigned int num)
{
    return (unsigned int)__builtin_ctz(num);
}

#define BIT(n) (1U << (n))
