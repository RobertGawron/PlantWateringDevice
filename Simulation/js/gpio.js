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
    // Update pump LED (GP2)
    const pumpLed = document.getElementById('pumpLed');
    if (pumpLed) {
        pumpLed.className = gpioState[2] ? 'led red-on' : 'led off';
    }
    
    // Update soil LED (GP1)
    const soilLed = document.getElementById('soilLed');
    if (soilLed) {
        soilLed.className = gpioState[1] ? 'led blue-on' : 'led off';
    }
    
    // Update display LED (GP0)
    const displayLed = document.getElementById('displayLed');
    if (displayLed) {
        displayLed.className = gpioState[0] ? 'led yellow-on' : 'led off';
    }
    
    // Update button LED (GP3 - active LOW, so pressed = 0)
    const buttonLed = document.getElementById('buttonLed');
    if (buttonLed) {
        buttonLed.className = (gpioState[3] === 0) ? 'led purple-on' : 'led off';
    }
    
    state.lastDisplayState = gpioState[0];
}

/**
 * Handle button press (active LOW)
 */
export function buttonDown() {
    gpioState[3] = 0; // Active LOW when pressed
    updateButtonLed(true);
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle button release (HIGH with pull-up)
 */
export function buttonUp() {
    gpioState[3] = 1; // HIGH when released
    updateButtonLed(false);
    addLog('Button released', 'debug-low');
}

/**
 * Update button LED state directly
 * @param {boolean} pressed - Whether button is pressed
 */
function updateButtonLed(pressed) {
    const buttonLed = document.getElementById('buttonLed');
    if (buttonLed) {
        buttonLed.className = pressed ? 'led purple-on' : 'led off';
    }
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