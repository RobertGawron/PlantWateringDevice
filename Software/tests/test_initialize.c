#include "unity.h"
#include "xc.h"
#include "watering.h"

extern void initialize(void);
extern PlantWateringData data;

/* Expected register values (must match production configuration) */
#define EXPECTED_OPTION_VALUE 0b11001111
#define EXPECTED_TRISGPIO_VALUE 0b00001010

#define TEST_GPIO_LEVEL_LOW (0U)

void setUp(void)
{
    /* Reset mock registers */
    OPTION = 0U;
    TRISGPIO = 0U;
    GPIObits = (GPIObits_t){0};

    /* Reset state */
    data.send_pulse_to_display = false;
}

void tearDown(void) {}

/**
 * @brief Verify hardware registers are configured correctly.
 *
 * @par Scenario:
 *  - Step 1: Ensure registers contain default values.
 *  - Step 2: Call initialize().
 *
 * @par Expected Result:
 *  - OPTION register is set to EXPECTED_OPTION_VALUE.
 *  - TRISGPIO register is set to EXPECTED_TRISGPIO_VALUE.
 */
void test_initialize_configures_registers(void)
{
    initialize();

    TEST_ASSERT_EQUAL_UINT8(EXPECTED_OPTION_VALUE, OPTION);
    TEST_ASSERT_EQUAL_UINT8(EXPECTED_TRISGPIO_VALUE, TRISGPIO);
}

/**
 * @brief Verify pump and display outputs are set to safe state.
 *
 * @par Scenario:
 *  - Step 1: Call initialize().
 *
 * @par Expected Result:
 *  - Pump MOSFET output is OFF.
 *  - Display output is LOW.
 */
void test_initialize_sets_outputs_to_safe_state(void)
{
    initialize();

    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_LOW, GPIObits.GP2);
    TEST_ASSERT_EQUAL_UINT8(TEST_GPIO_LEVEL_LOW, GPIObits.GP0);
}

/**
 * @brief Verify startup display pulse is scheduled.
 *
 * @par Scenario:
 *  - Step 1: Call initialize().
 *
 * @par Expected Result:
 *  - send_pulse_to_display flag is set to true.
 */
void test_initialize_sets_startup_display_pulse(void)
{
    initialize();

    TEST_ASSERT_TRUE(data.send_pulse_to_display);
}

/* Unity test runner */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_initialize_configures_registers);
    RUN_TEST(test_initialize_sets_outputs_to_safe_state);
    RUN_TEST(test_initialize_sets_startup_display_pulse);

    return UNITY_END();
}