#include <cstdint>
#include <cstdio>

#include <unity.h>

#include "filters/kalman.h"

void test_kalman() {
    uint16_t in[] = {
        325, 325, 320, 319, 320, 320, 320, 320, 319, 319, 319, 319, 318, 319, 321, 330, 321,
        321, 324, 321, 321, 322, 324, 322, 322, 319, 319, 319, 319, 319, 319, 319, 319, 319,
        321, 324, 321, 321, 322, 324, 322, 322, 324, 324, 324, 324, 324, 324, 324, 324, 324,
    };

    constexpr size_t in_size = sizeof(in) / sizeof(in[0]);

    uint16_t out[in_size];
    uint16_t exp_out[in_size] = {
        170, 230, 259, 277, 289, 298, 304, 308, 311, 313, 315, 316, 317, 317, 318, 321, 321,
        321, 322, 322, 322, 322, 322, 322, 322, 321, 321, 320, 320, 320, 319, 319, 319, 319,
        320, 321, 321, 321, 321, 322, 322, 322, 323, 323, 323, 323, 324, 324, 324, 324, 324,
    };

    KalmanFilterClass filter;
    filter.setCovs(0.1, 1.0);

    for (int i = 0; i < in_size; i++) {
        out[i] = filter.update(in[i]);
        // printf("%u\n", filter.update(in[i]));
    }
    char msg[80];
    snprintf(msg, sizeof(msg), "Q=0.1, R=1.0, %u samples", in_size);
    TEST_MESSAGE(msg);

    TEST_ASSERT_EQUAL_UINT16_ARRAY(exp_out, out, in_size);
}