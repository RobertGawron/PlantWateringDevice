import { state, gpioState } from './state.js';
import { addLog } from './logging.js';

/**
 * Get GPIO pin state (called from C code)
 * @param {number} pin - GPIO pin number
 * @returns {number} Pin state (0 or 1)
 */
export function getGPIOState(pin) {
    return gpioState[pin] || 0;
}

/**
 * Set GPIO pin state (called from C code)
 * @param {number} pin - GPIO pin number
 * @param {number} pinState - Pin state (0 or 1)
 */
export function setGPIOState(pin, pinState) {
    gpioState[pin] = pinState;
    updateDisplayFromGPIO();
}

/**
 * Update UI elements when GPIO changes
 */
export function updateDisplayFromGPIO() {
    // Update pump LED
    const pumpLed = document.getElementById('pumpLed');
    if (pumpLed) {
        pumpLed.className = gpioState[2] ? 'led red-on' : 'led off';
    }
    
    // Update soil LED
    const soilLed = document.getElementById('soilLed');
    if (soilLed) {
        soilLed.className = gpioState[1] ? 'led blue-on' : 'led off';
    }
    
    // Update display LED
    const displayLed = document.getElementById('displayLed');
    if (displayLed) {
        displayLed.className = gpioState[0] ? 'led yellow-on' : 'led off';
    }
    
    state.lastDisplayState = gpioState[0];
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
    addLog(`Soil sensor: ${state.soilDry ? 'DRY' : 'WET'}`, 'info');
    updateDisplayFromGPIO();
}