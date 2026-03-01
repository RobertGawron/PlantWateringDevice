#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void handle_pump(void);
extern PlantWateringData data;

void setUp(void)
{
    GPIObits.GP2 = 0;
}

void tearDown(void) {}

void test_pump_turns_off_when_no_remaining_level(void)
{
    data.pump.remaining_level = 0;
    data.pump.level_remaining_seconds = 5;

    handle_pump();

    TEST_ASSERT_EQUAL(0, GPIObits.GP2);
}

void test_pump_turns_on_when_active(void)
{
    data.pump.remaining_level = 2;
    data.pump.level_remaining_seconds = 5;

    handle_pump();

    TEST_ASSERT_EQUAL(1, GPIObits.GP2);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_pump_turns_off_when_no_remaining_level);
    RUN_TEST(test_pump_turns_on_when_active);

    return UNITY_END();
}