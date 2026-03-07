import { state, gpioState } from './state.js';
import { addLog } from './logging.js';

/**
 * Get GPIO pin state (called from C code via EM_JS)
 */
export function getGPIOState(pin) {
    const value = gpioState[pin];
   
    // Return 0 or 1, never undefined
    if (value === undefined) {
        console.error(`[JS] ERROR: gpioState[${pin}] is undefined!`);
        return 0;
    }
   
    return value != 0;
}

/**
 * Set GPIO pin state (called from C code via EM_JS)
 */
export function setGPIOState(pin, pinState) {
    gpioState[pin] = pinState;
    // No need to update display LEDs anymore
}

/**
 * Update UI elements when GPIO changes (kept for future use if needed)
 */
export function updateDisplayFromGPIO() {
    // All LED indicators removed
    // This function is kept in case you want to add other UI updates later
}

/**
 * Handle button press (active LOW)
 */
export function buttonDown() {
    gpioState[3] = 0;
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle button release (HIGH with pull-up)
 */
export function buttonUp() {
    gpioState[3] = 1;
    addLog('Button released', 'debug-low');
}

/**
 * Toggle soil moisture sensor state
 */
export function toggleSoil() {
    state.soilDry = !state.soilDry;
    gpioState[1] = state.soilDry ? 1 : 0;
    console.log(`[JS] toggleSoil() - gpioState[1] = ${gpioState[1]}`);
    addLog(`Soil sensor: ${state.soilDry ? 'DRY' : 'WET'}`, 'info');
}