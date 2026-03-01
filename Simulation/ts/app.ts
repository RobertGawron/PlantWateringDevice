import { state, gpioState } from './state';
import type { GPIOPinLevel } from './state';
import { addLog, clearLog } from './logging';
import { buttonDown, buttonUp, toggleSoil, getGPIOState, setGPIOState } from './gpio';
import {
    startSimulation,
    stopSimulation,
    toggleSimulation,
    updateSpeed,
    singleStep,
    runMinute,
    runHour
} from './simulation';
import { initGraph, clearGraph } from './graph';

// ------------------------------------------------------------
// Type Definitions
// ------------------------------------------------------------

/**
 * Emscripten WASM Module interface.
 * 
 * Defines the functions exported from C code compiled with Emscripten,
 * plus the GPIO functions we inject from JavaScript.
 */
export interface EmscriptenModule {
    /** Initialize the firmware */
    _main: () => void;
    /** Advance simulation by one tick (20ms) */
    _advanceTick: () => void;
    /** Get GPIO pin state - injected from JS for C code access */
    getGPIOState: (pin: number) => GPIOPinLevel;
    /** Set GPIO pin state - injected from JS for C code access */
    setGPIOState: (pin: number, pinState: number) => void;
}

/**
 * Extend Window interface with global functions exposed for:
 * - HTML inline event handlers (onclick, onmousedown, etc.)
 * - Emscripten EM_JS C-to-JavaScript interop
 */
declare global {
    interface Window {
        // GPIO state shared with Emscripten early stub
        _gpioState: typeof gpioState;

        // Emscripten module
        Module: Partial<EmscriptenModule>;
        createPlantWateringModule: (config: Partial<EmscriptenModule>) => Promise<EmscriptenModule>;

        // GPIO functions (called from C via EM_JS)
        getGPIOState: typeof getGPIOState;
        setGPIOState: typeof setGPIOState;

        // UI control functions (called from HTML onclick)
        buttonDown: typeof buttonDown;
        buttonUp: typeof buttonUp;
        toggleSoil: typeof toggleSoil;

        // Simulation control functions (called from HTML onclick)
        startSimulation: typeof startSimulation;
        stopSimulation: typeof stopSimulation;
        toggleSimulation: typeof toggleSimulation;
        singleStep: typeof singleStep;
        runMinute: typeof runMinute;
        runHour: typeof runHour;

        // Utility functions (called from HTML onclick)
        clearGraph: typeof clearGraph;
        clearLog: typeof clearLog;
    }
}

// ------------------------------------------------------------
// Global Function Exposure
// ------------------------------------------------------------

/**
 * Expose functions to the global window object.
 * 
 * Required for:
 * 1. HTML inline event handlers (onclick="toggleSimulation()")
 * 2. Emscripten EM_JS calls to getGPIOState/setGPIOState
 * 
 * Must be called before WASM module loads to ensure the GPIO
 * stub functions are available for C code initialization.
 */
function exposeGlobalFunctions(): void {
    // GPIO state for Emscripten early stub access
    window._gpioState = gpioState;

    // GPIO functions for C code interop
    window.getGPIOState = getGPIOState;
    window.setGPIOState = setGPIOState;

    // Update existing Module stub if present
    if (window.Module) {
        window.Module.getGPIOState = getGPIOState;
        window.Module.setGPIOState = setGPIOState;
    }

    // UI control functions
    window.buttonDown = buttonDown;
    window.buttonUp = buttonUp;
    window.toggleSoil = toggleSoil;

    // Simulation control functions
    window.startSimulation = startSimulation;
    window.stopSimulation = stopSimulation;
    window.toggleSimulation = toggleSimulation;
    window.singleStep = singleStep;
    window.runMinute = runMinute;
    window.runHour = runHour;

    // Utility functions
    window.clearGraph = clearGraph;
    window.clearLog = clearLog;
}

// ------------------------------------------------------------
// Module Loading
// ------------------------------------------------------------

/**
 * Load and initialize the WebAssembly module.
 * 
 * This function:
 * 1. Calls the Emscripten-generated factory function
 * 2. Attaches GPIO functions for C code access
 * 3. Calls _main() to initialize the firmware
 * 
 * @throws Error if createPlantWateringModule is not available
 */
async function loadModule(): Promise<void> {
    if (typeof window.createPlantWateringModule !== 'function') {
        const error = new Error(
            'createPlantWateringModule not found. ' +
            'Ensure plant-watering.js is loaded before app.js'
        );
        addLog(error.message, 'error');
        throw error;
    }

    try {
        const module = await window.createPlantWateringModule(window.Module);

        // Attach GPIO functions to module for C code access
        module.getGPIOState = getGPIOState;
        module.setGPIOState = setGPIOState;

        // Store references
        state.Module = module;
        window.Module = module;

        addLog('Module loaded successfully', 'info');

        // Initialize the firmware
        module._main();

    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        addLog(`Failed to load module: ${message}`, 'error');
        throw error;
    }
}

// ------------------------------------------------------------
// Event Listeners
// ------------------------------------------------------------

/**
 * Set up DOM event listeners that cannot be defined inline in HTML.
 * 
 * Note: Most UI interactions use inline handlers in HTML for simplicity.
 * This function handles events that require JavaScript-only binding.
 */
function setupEventListeners(): void {
    const speedSlider = document.getElementById('speedSlider');
    if (speedSlider) {
        speedSlider.addEventListener('input', updateSpeed);
    }
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

/**
 * Initialize the application.
 * 
 * Initialization order is important:
 * 1. Expose globals - must happen before WASM loads
 * 2. Initialize graph - prepares canvas elements
 * 3. Setup event listeners - binds DOM events
 * 4. Load WASM module - initializes firmware (async)
 */
async function init(): Promise<void> {
    console.log('[APP] Initializing Plant Watering Simulator');

    exposeGlobalFunctions();
    initGraph();
    setupEventListeners();

    try {
        await loadModule();
        console.log('[APP] Initialization complete');
    } catch (error) {
        console.error('[APP] Initialization failed:', error);
    }
}

// Start application when DOM is ready
window.addEventListener('load', init);

// ------------------------------------------------------------
// Exports
// ------------------------------------------------------------

export { init, loadModule };