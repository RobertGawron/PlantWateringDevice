import { state, gpioState, GPIOPin, type GPIOPinLevel } from './state';
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
 * Emscripten WASM Module interface
 */
export interface EmscriptenModule {
    _main: () => void;
    _advanceTick: () => void;
    getGPIOState: (pin: number) => GPIOPinLevel;
    setGPIOState: (pin: number, state: number) => void;
}

/**
 * Extend Window interface with our global functions
 */
declare global {
    interface Window {
        // GPIO state for early stub
        _gpioState: typeof gpioState;
        
        // Module
        Module: Partial<EmscriptenModule>;
        createPlantWateringModule: (module: Partial<EmscriptenModule>) => Promise<EmscriptenModule>;
        
        // GPIO functions
        getGPIOState: typeof getGPIOState;
        setGPIOState: typeof setGPIOState;
        
        // Control functions
        buttonDown: typeof buttonDown;
        buttonUp: typeof buttonUp;
        toggleSoil: typeof toggleSoil;
        
        // Simulation functions
        startSimulation: typeof startSimulation;
        stopSimulation: typeof stopSimulation;
        toggleSimulation: typeof toggleSimulation;
        singleStep: typeof singleStep;
        runMinute: typeof runMinute;
        runHour: typeof runHour;
        
        // Graph functions
        clearGraph: typeof clearGraph;
        
        // Logging functions
        clearLog: typeof clearLog;
    }
    
    // For globalThis access
    var getGPIOState: typeof import('./gpio').getGPIOState;
    var setGPIOState: typeof import('./gpio').setGPIOState;
}

// ------------------------------------------------------------
// Global Function Exposure
// ------------------------------------------------------------

/**
 * Expose all functions to global scope for HTML onclick handlers
 * and Emscripten C code interop
 */
function exposeGlobalFunctions(): void {
    // Share gpioState with the early stub
    window._gpioState = gpioState;

    // Replace Module functions with the real implementations
    if (typeof window.Module !== 'undefined' && window.Module) {
        window.Module.getGPIOState = getGPIOState;
        window.Module.setGPIOState = setGPIOState;
        console.log('[APP] Module GPIO functions updated with real implementations');
    }

    // Also attach to global/window for other code
    globalThis.getGPIOState = getGPIOState;
    globalThis.setGPIOState = setGPIOState;

    // Controls
    window.buttonDown = buttonDown;
    window.buttonUp = buttonUp;
    window.toggleSoil = toggleSoil;

    // Simulation
    window.startSimulation = startSimulation;
    window.stopSimulation = stopSimulation;
    window.toggleSimulation = toggleSimulation;
    window.singleStep = singleStep;
    window.runMinute = runMinute;
    window.runHour = runHour;

    // Graph
    window.clearGraph = clearGraph;

    // Logging
    window.clearLog = clearLog;

    console.log('[APP] Global functions exposed');
}

// ------------------------------------------------------------
// Module Loading
// ------------------------------------------------------------

/**
 * Load the WebAssembly module
 */
async function loadModule(): Promise<void> {
    try {
        if (typeof window.createPlantWateringModule !== 'function') {
            throw new Error('createPlantWateringModule not found. Is plant-watering.js loaded?');
        }

        const module = await window.createPlantWateringModule(window.Module);

        // Store module in state
        state.Module = module;

        // Attach GPIO functions to the module
        state.Module.getGPIOState = getGPIOState;
        state.Module.setGPIOState = setGPIOState;

        // Update global reference
        window.Module = state.Module;

        console.log('[APP] Functions re-attached after module creation');

        addLog('Module loaded successfully', 'info');

        // Call main to initialize the firmware
        state.Module._main();

    } catch (error) {
        const errorMessage = error instanceof Error ? error.message : String(error);
        addLog(`Failed to load module: ${errorMessage}`, 'error');
        console.error('[APP] Module load error:', error);
    }
}

// ------------------------------------------------------------
// Event Listeners
// ------------------------------------------------------------

/**
 * Setup DOM event listeners
 */
function setupEventListeners(): void {
    // Speed slider
    const speedSlider = document.getElementById('speedSlider');
    if (speedSlider) {
        speedSlider.addEventListener('input', updateSpeed);
    }

    console.log('[APP] Event listeners setup complete');
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

/**
 * Initialize the application
 */
function init(): void {
    console.log('[APP] Initializing Plant Watering Simulator...');

    // Expose all functions to global scope FIRST
    exposeGlobalFunctions();

    // Initialize graph
    initGraph();

    // Setup event listeners
    setupEventListeners();

    // Load WASM module (simulation NOT started automatically)
    loadModule().catch((error) => {
        console.error('[APP] Failed to load module during init:', error);
    });

    console.log('[APP] Initialization complete');
}

// Initialize on page load
window.addEventListener('load', init);

// ------------------------------------------------------------
// Exports
// ------------------------------------------------------------

export { init, loadModule };