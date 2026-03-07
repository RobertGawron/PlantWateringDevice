import { state, gpioState } from './state.js';
import { addLog } from './logging.js';

/**
 * Get GPIO pin state (called from C code via EM_JS)
 */
export function getGPIOState(pin) {
    const value = gpioState[pin];
   
    // Debug: log every read
    console.log(`[JS] getGPIOState(${pin}) = ${value}, gpioState =`, JSON.stringify(gpioState));
    
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
    console.log(`[JS] setGPIOState(${pin}, ${pinState})`);
    gpioState[pin] = pinState;
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
    console.log('[JS] buttonDown() called');
    gpioState[3] = 0;
    updateDisplayFromGPIO();
    addLog('Button pressed', 'debug-low');
}

/**
 * Handle button release (HIGH with pull-up)
 */
export function buttonUp() {
    console.log('[JS] buttonUp() called');
    gpioState[3] = 1;
    updateDisplayFromGPIO();
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
    updateDisplayFromGPIO();
}

/**
 * Debug: get current GPIO state
 */
export function debugGPIOState() {
    console.log('[JS] gpioState:', JSON.stringify(gpioState));
    console.log('[JS] state.soilDry:', state.soilDry);
    return gpioState;
}