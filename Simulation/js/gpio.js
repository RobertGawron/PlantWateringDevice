import { state, gpioState } from './state.js';
import { addLog } from './logging.js';

// Debug flag - set to false to reduce console spam
const GPIO_DEBUG = false;

/**
 * Get GPIO pin state (called from C code via EM_JS)
 */
export function getGPIOState(pin) {
    const value = gpioState[pin] || 0;
    
    if (GPIO_DEBUG) {
        console.log(`[GPIO] GET pin=${pin} -> ${value}`);
    }
    
    return value;
}

/**
 * Set GPIO pin state (called from C code via EM_JS)
 */
export function setGPIOState(pin, pinState) {
    const oldState = gpioState[pin];
    gpioState[pin] = pinState;
    
    if (GPIO_DEBUG && oldState !== pinState) {
        console.log(`[GPIO] SET pin=${pin}: ${oldState} -> ${pinState}`);
    }
    
    updateDisplayFromGPIO();
}

/**
 * Update UI elements when GPIO changes
 */
export function updateDisplayFromGPIO() {
    // GP0 - Display clock
    const displayLed = document.getElementById('displayLed');
    if (displayLed) {
        displayLed.className = gpioState[0] ? 'led yellow-on' : 'led off';
    }
    
    // GP1 - Soil sensor (HIGH = dry)
    const soilLed = document.getElementById('soilLed');
    if (soilLed) {
        soilLed.className = gpioState[1] ? 'led blue-on' : 'led off';
    }
    
    // GP2 - Pump (HIGH = on)
    const pumpLed = document.getElementById('pumpLed');
    if (pumpLed) {
        pumpLed.className = gpioState[2] ? 'led red-on' : 'led off';
    }
    
    // GP3 - Button (LOW = pressed, HIGH = idle)
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
    gpioState[3] = 0;
    updateButtonLed(true);
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle button release (HIGH with pull-up)
 */
export function buttonUp() {
    gpioState[3] = 1;
    updateButtonLed(false);
    addLog('Button released', 'debug-low');
}

/**
 * Update button LED state directly
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
    console.log(`[GPIO] Soil toggled - GP1 = ${gpioState[1]} (${state.soilDry ? 'DRY' : 'WET'})`);
    addLog(`Soil sensor: ${state.soilDry ? 'DRY' : 'WET'}`, 'info');
    updateDisplayFromGPIO();
}

/**
 * Debug: get current GPIO state
 */
export function debugGPIOState() {
    console.log('[GPIO] Current state:', JSON.stringify(gpioState));
    return gpioState;
}