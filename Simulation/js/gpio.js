import { state, gpioState } from './state.js';
import { addLog } from './logging.js';
import { updateSevenSegment } from './ui.js';

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
    const previousState = gpioState[pin];
    gpioState[pin] = pinState;

    // Detect rising edge on GP0 (Display clock)
    if (pin === 0 && previousState === 0 && pinState === 1) {
        state.currentDisplayValue++;
        updateSevenSegment();
    }
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