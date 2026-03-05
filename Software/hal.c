#include "hal.h"
volatile uint8_t OPTION;
volatile uint8_t TRISGPIO;
volatile GPIObits_t GPIObits;


EM_JS(int, js_get_gpio, (int pin), {
    if (typeof getGPIOState === 'function')
    {
        return getGPIOState(pin);
    }
    return 0;
});

EM_JS(void, js_set_gpio, (int pin, int state), {
    if (typeof setGPIOState === 'function')
    {
        setGPIOState(pin, state);
    }
});

// Simple blocking implementation using tick_gate
volatile int tick_gate = 0;

void HW_DELAY_MS(int DURATION_MS)
{
    tick_gate = 0;
    
    // Block until JavaScript sets tick_gate = 1
    while (tick_gate == 0)
    {
        emscripten_sleep(1); // Yields to browser
    }
}

EMSCRIPTEN_KEEPALIVE
void advanceTick(void)
{
    tick_gate = 1; // Unblock HW_DELAY_MS
}

void GPIO_SET(uint8_t GPIO_PIN, uint8_t STATE)
{
    js_set_gpio(GPIO_PIN, STATE);
}

bool GPIO_GET(uint8_t GPIO_PIN)
{
    return js_get_gpio(GPIO_PIN);
}