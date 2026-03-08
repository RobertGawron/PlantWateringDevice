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
import { initGraph, clearGraph, addGraphDataFromGPIO } from './graph';

// ------------------------------------------------------------
// Type Definitions
// ------------------------------------------------------------

/**
 * Emscripten WASM Module interface.
 * 
 * Defines the functions exported from C code compiled with Emscripten.
 * GPIO functions are called via window object (EM_JS), not module.
 */
export interface EmscriptenModule {
    /** Initialize the firmware */
    _main: () => void;
}

/**
 * Extend Window interface with global functions exposed for:
 * - HTML inline event handlers (onclick, onmousedown, etc.)
 * - Emscripten EM_JS C-to-JavaScript interop
 */
declare global {
    interface Window {
        // GPIO state shared with Emscripten
        _gpioState: typeof gpioState;

        // Emscripten module
        Module: Partial<EmscriptenModule>;
        createPlantWateringModule: (config?: Partial<EmscriptenModule>) => Promise<EmscriptenModule>;

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
 * functions are available for C code.
 */
function exposeGlobalFunctions(): void {
    // GPIO state for debugging
    window._gpioState = gpioState;

    // GPIO functions - called by EM_JS from C code
    window.getGPIOState = getGPIOState;
    window.setGPIOState = setGPIOState;

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

        const module = await window.createPlantWateringModule();

        state.Module = module;
        window.Module = module;

        addLog('Module loaded successfully', 'info');
    console.log('[DEBUG] Before first _main(), gpioState:', { ...gpioState });

        // Run initialization tick (tick 0)
        module._main();
            console.log('[DEBUG] After first _main(), gpioState:', { ...gpioState });
    console.log('[DEBUG] Display pin (0):', gpioState[0]);
        // Record state at tick 0 (t=0)
        // After first _main(): display is HIGH from initial pulse
        addGraphDataFromGPIO({ ...gpioState }, 0);
        state.tickCount = 1;

        addLog('Module loaded successfully', 'info');

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
 * 4. Load WASM module - starts firmware (async)
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