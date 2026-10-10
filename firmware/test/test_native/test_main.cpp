#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_kalman();

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_kalman);

    return UNITY_END();
}
