#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void handle_display(void);
extern PlantWateringData data;

#define DISPLAY_PIN GPIObits.GP0

#define TEST_GPIO_LEVEL_LOW (0U)
#define TEST_GPIO_LEVEL_HIGH (1U)

void setUp(void)
{
    DISPLAY_PIN = TEST_GPIO_LEVEL_LOW;

    data.send_pulse_to_display = false;
    data.sending_pulse_to_display = false;
}

void tearDown(void) {}

/**
 * @brief Verify no output change when no pulse requested.
 *
 * @par Scenario:
 *  - Step 1: Ensure both flags are false.
 *  - Step 2: Call handle_display().
 *
 * @par Expected Result:
 *  - Display output remains LOW.
 *  - Flags remain unchanged.
 */
void test_no_pulse_when_flags_false(void)
{
    handle_display();

    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_LOW, DISPLAY_PIN);
    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/**
 * @brief Verify first invocation generates HIGH pulse.
 *
 * @par Scenario:
 *  - Step 1: Set send_pulse_to_display = true.
 *  - Step 2: Call handle_display().
 *
 * @par Expected Result:
 *  - Output becomes HIGH.
 *  - send_pulse_to_display cleared.
 *  - sending_pulse_to_display set.
 */
void test_first_pulse_sets_high(void)
{
    data.send_pulse_to_display = true;

    handle_display();

    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_HIGH, DISPLAY_PIN);
    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_TRUE(data.sending_pulse_to_display);
}

/**
 * @brief Verify second invocation completes pulse (sets LOW).
 *
 * @par Scenario:
 *  - Step 1: Set sending_pulse_to_display = true.
 *  - Step 2: Call handle_display().
 *
 * @par Expected Result:
 *  - Output becomes LOW.
 *  - sending_pulse_to_display cleared.
 */
void test_second_pulse_sets_low(void)
{
    data.sending_pulse_to_display = true;
    DISPLAY_PIN = TEST_GPIO_LEVEL_HIGH;

    handle_display();

    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_LOW, DISPLAY_PIN);
    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/**
 * @brief Verify full pulse sequence over two invocations.
 *
 * @par Scenario:
 *  - Step 1: Set send_pulse_to_display = true.
 *  - Step 2: Call handle_display() → expect HIGH.
 *  - Step 3: Call handle_display() again → expect LOW.
 *
 * @par Expected Result:
 *  - First call sets HIGH.
 *  - Second call sets LOW.
 *  - Both flags cleared.
 */
void test_full_pulse_sequence(void)
{
    data.send_pulse_to_display = true;

    /* First tick */
    handle_display();
    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_HIGH, DISPLAY_PIN);

    /* Second tick */
    handle_display();
    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_LOW, DISPLAY_PIN);

    TEST_ASSERT_FALSE(data.send_pulse_to_display);
    TEST_ASSERT_FALSE(data.sending_pulse_to_display);
}

/* Unity runner */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_no_pulse_when_flags_false);
    RUN_TEST(test_first_pulse_sets_high);
    RUN_TEST(test_second_pulse_sets_low);
    RUN_TEST(test_full_pulse_sequence);

    return UNITY_END();
}