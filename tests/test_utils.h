#pragma once

#include <float.h>
#include <math.h>
#include <stdio.h>

#define RUN_TEST(f)                                                                                                    \
    do {                                                                                                               \
        if (f()) {                                                                                                     \
            printf("[FAILED] %s\n", #f);                                                                               \
            failures++;                                                                                                \
        } else {                                                                                                       \
            printf("[SUCCESS] %s\n", #f);                                                                              \
        }                                                                                                              \
    } while (0)

#define ASSERT(x)                                                                                                      \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            printf("Assertion (%s) failed at line %d in file %s\n", #x, __LINE__, __FILE__);                           \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

#define APPROX_EQ(real, expected) ASSERT(fabsf((expected) - (real)) < FLT_EPSILON)

#define APPROX_EQ_DBL(real, expected) ASSERT(fabs((expected) - (real)) < FLT_EPSILON)
