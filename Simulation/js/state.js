// Global application state
export const state = {
    Module: null,
    isRunning: false,
    intervalId: null,
    tickCount: 0,
    currentDisplayValue: 1,
    lastPumpState: 0,
    lastDisplayState: 0,
    soilDry: false
};

// GPIO state storage
export const gpioState = {
    0: 0, // GP0 - Display
    1: 0, // GP1 - Soil sensor
    2: 0, // GP2 - Pump
    3: 1  // GP3 - Button (default HIGH with pull-up)
};