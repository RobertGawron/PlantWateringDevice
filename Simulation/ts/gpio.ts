import {
    state,
    gpioState,
    GPIOPin,
    GPIO_PIN_HIGH,
    GPIO_PIN_LOW
} from './state';
import type { GPIOPinLevel } from './state';
import { addLog } from './logging';
import { updateSevenSegment } from './ui';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

/** Valid GPIO pin numbers for this hardware */
const VALID_GPIO_PINS = new Set([
    GPIOPin.DISPLAY,
    GPIOPin.SOIL_SENSOR,
    GPIOPin.PUMP,
    GPIOPin.BUTTON
]);

// ------------------------------------------------------------
// Validation Helpers
// ------------------------------------------------------------

/**
 * Check if a pin number is a valid GPIO pin.
 * 
 * @param pin - Pin number to validate
 * @returns true if the pin is valid
 */
function isValidPin(pin: number): pin is GPIOPin {
    return VALID_GPIO_PINS.has(pin);
}

/**
 * Normalize a numeric value to a GPIO pin level.
 * 
 * @param value - Any numeric value
 * @returns GPIO_PIN_HIGH for non-zero, GPIO_PIN_LOW for zero
 */
function toPinLevel(value: number): GPIOPinLevel {
    return value !== 0 ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

// ------------------------------------------------------------
// GPIO State Functions (called from C code via EM_JS)
// ------------------------------------------------------------

/**
 * Get GPIO pin state.
 * 
 * Called from C code via Emscripten EM_JS bindings.
 * 
 * @param pin - Pin number (0-3 for this hardware)
 * @returns GPIO_PIN_HIGH (1) or GPIO_PIN_LOW (0)
 */
export function getGPIOState(pin: number): GPIOPinLevel {
    if (!isValidPin(pin)) {
        console.error(`[GPIO] Invalid pin number: ${pin}`);
        return GPIO_PIN_LOW;
    }

    return gpioState[pin];
}

/**
 * Set GPIO pin state.
 * 
 * Called from C code via Emscripten EM_JS bindings.
 * Detects rising edge on display pin to update the seven-segment display.
 * 
 * @param pin - Pin number (0-3 for this hardware)
 * @param pinState - Desired state (0 = LOW, non-zero = HIGH)
 */
export function setGPIOState(pin: number, pinState: number): void {
    if (!isValidPin(pin)) {
        console.error(`[GPIO] Invalid pin number: ${pin}`);
        return;
    }

    const previousState = gpioState[pin];
    const newState = toPinLevel(pinState);

    gpioState[pin] = newState;

    // Detect rising edge on display clock pin
    if (pin === GPIOPin.DISPLAY &&
        previousState === GPIO_PIN_LOW &&
        newState === GPIO_PIN_HIGH) {
        state.currentDisplayValue++;
        updateSevenSegment();
    }
}

// ------------------------------------------------------------
// UI Control Functions (called from HTML event handlers)
// ------------------------------------------------------------

/**
 * Handle hardware button press.
 * 
 * The button uses active-low logic (pressed = LOW, released = HIGH)
 * due to the internal pull-up resistor configuration.
 */
export function buttonDown(): void {
    gpioState[GPIOPin.BUTTON] = GPIO_PIN_LOW;
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle hardware button release.
 * 
 * Returns to HIGH state due to pull-up resistor.
 */
export function buttonUp(): void {
    gpioState[GPIOPin.BUTTON] = GPIO_PIN_HIGH;
    addLog('Button released', 'debug-low');
}

/**
 * Toggle soil moisture sensor state between wet and dry.
 * 
 * Simulates changing soil conditions:
 * - DRY (HIGH): Soil moisture below threshold, pump should activate
 * - WET (LOW): Soil moisture adequate, pump should remain off
 */
export function toggleSoil(): void {
    state.soilDry = !state.soilDry;
    gpioState[GPIOPin.SOIL_SENSOR] = state.soilDry ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
    addLog(`Soil sensor: ${state.soilDry ? 'DRY' : 'WET'}`, 'info');
}