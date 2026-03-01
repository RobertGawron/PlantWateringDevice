// GPIO Pin voltage levels
export const GPIO_PIN_LOW = 0;
export const GPIO_PIN_HIGH = 1;

export type GPIOPinLevel = typeof GPIO_PIN_LOW | typeof GPIO_PIN_HIGH;

// GPIO Pin definitions
export enum GPIOPin {
    DISPLAY = 0,      // GP0 - Display
    SOIL_SENSOR = 1,  // GP1 - Soil sensor
    PUMP = 2,         // GP2 - Pump
    BUTTON = 3        // GP3 - Button (default HIGH with pull-up)
}

// GPIO state storage
export type GPIOStateMap = {
    [key in GPIOPin]: GPIOPinLevel;
};

export const gpioState: GPIOStateMap = {
    [GPIOPin.DISPLAY]: GPIO_PIN_LOW,      // GP0 - Display
    [GPIOPin.SOIL_SENSOR]: GPIO_PIN_LOW,  // GP1 - Soil sensor
    [GPIOPin.PUMP]: GPIO_PIN_LOW,         // GP2 - Pump
    [GPIOPin.BUTTON]: GPIO_PIN_HIGH       // GP3 - Button (default HIGH with pull-up)
};

// Global application state
export interface AppState {
    Module: unknown;  // Will be the WASM module, typed properly in wasm-loader.ts
    isRunning: boolean;
    intervalId: number | null;
    tickCount: number;
    currentDisplayValue: number;
    soilDry: boolean;
}

export const state: AppState = {
    Module: null,
    isRunning: false,
    intervalId: null,
    tickCount: 0,
    currentDisplayValue: 0,
    soilDry: false
};

// Helper functions for type-safe GPIO access
export function getGPIOValue(pin: GPIOPin): GPIOPinLevel {
    return gpioState[pin];
}

export function setGPIOValue(pin: GPIOPin, value: GPIOPinLevel): void {
    gpioState[pin] = value;
}

export function toggleGPIOValue(pin: GPIOPin): void {
    gpioState[pin] = gpioState[pin] === GPIO_PIN_HIGH ? GPIO_PIN_LOW : GPIO_PIN_HIGH;
}

export function isGPIOHigh(pin: GPIOPin): boolean {
    return gpioState[pin] === GPIO_PIN_HIGH;
}

export function isGPIOLow(pin: GPIOPin): boolean {
    return gpioState[pin] === GPIO_PIN_LOW;
}