#include "unity.h"

/* Required by Unity */
void setUp(void) {}
void tearDown(void) {}

/* Dummy function under test */
int add(int a, int b)
{
    return a + b;
}

/* Test cases */
void test_add_positive_numbers(void)
{
    TEST_ASSERT_EQUAL_INT(5, add(2, 3));
}

void test_add_negative_numbers(void)
{
    TEST_ASSERT_EQUAL_INT(-5, add(-2, -3));
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_add_positive_numbers);
    RUN_TEST(test_add_negative_numbers);

    return UNITY_END();
}