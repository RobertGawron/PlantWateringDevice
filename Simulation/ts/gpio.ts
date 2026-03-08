import { 
    state, 
    gpioState, 
    GPIOPin, 
    GPIO_PIN_HIGH, 
    GPIO_PIN_LOW,
    type GPIOPinLevel 
} from './state';
import { addLog } from './logging';
import { updateSevenSegment } from './ui';

/**
 * Get GPIO pin state (called from C code via EM_JS)
 * @param pin - Pin number (raw number from C code)
 * @returns 0 or 1
 */
export function getGPIOState(pin: number): GPIOPinLevel {
    // Validate pin is a known GPIO pin
    if (!(pin in GPIOPin)) {
        console.error(`[JS] ERROR: Invalid GPIO pin: ${pin}`);
        return GPIO_PIN_LOW;
    }

    const value = gpioState[pin as GPIOPin];

    // Return 0 or 1, never undefined
    if (value === undefined) {
        console.error(`[JS] ERROR: gpioState[${pin}] is undefined!`);
        return GPIO_PIN_LOW;
    }

    return value !== GPIO_PIN_LOW ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
}

/**
 * Set GPIO pin state (called from C code via EM_JS)
 * @param pin - Pin number (raw number from C code)
 * @param pinState - Pin state (0 or 1)
 */
export function setGPIOState(pin: number, pinState: number): void {
    // Validate pin
    if (!(pin in GPIOPin)) {
        console.error(`[JS] ERROR: Invalid GPIO pin: ${pin}`);
        return;
    }

    const gpioPin = pin as GPIOPin;
    const previousState = gpioState[gpioPin];
    const newState: GPIOPinLevel = pinState !== 0 ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
    
    gpioState[gpioPin] = newState;

    // Detect rising edge on GP0 (Display clock)
    if (gpioPin === GPIOPin.DISPLAY && 
        previousState === GPIO_PIN_LOW && 
        newState === GPIO_PIN_HIGH) {
        state.currentDisplayValue++;
        updateSevenSegment();
    }
}

/**
 * Handle button press (active LOW)
 */
export function buttonDown(): void {
    gpioState[GPIOPin.BUTTON] = GPIO_PIN_LOW;
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle button release (HIGH with pull-up)
 */
export function buttonUp(): void {
    gpioState[GPIOPin.BUTTON] = GPIO_PIN_HIGH;
    addLog('Button released', 'debug-low');
}

/**
 * Toggle soil moisture sensor state
 */
export function toggleSoil(): void {
    state.soilDry = !state.soilDry;
    gpioState[GPIOPin.SOIL_SENSOR] = state.soilDry ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
    console.log(`[JS] toggleSoil() - gpioState[${GPIOPin.SOIL_SENSOR}] = ${gpioState[GPIOPin.SOIL_SENSOR]}`);
    addLog(`Soil sensor: ${state.soilDry ? 'DRY' : 'WET'}`, 'info');
}