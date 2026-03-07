import { state, gpioState } from './state.js';
import { addLog, clearLog } from './logging.js';
import { buttonDown, buttonUp, toggleSoil, getGPIOState, setGPIOState } from './gpio.js';
import { startSimulation, stopSimulation, toggleSimulation, updateSpeed, singleStep, runMinute, runHour } from './simulation.js';
import { initGraph, clearGraph } from './graph.js';

/**
 * Expose all functions to global scope
 */
function exposeGlobalFunctions() {
    // Share gpioState with the early stub
    window._gpioState = gpioState;
    
    // Replace Module functions with the real implementations
   if (typeof Module !== 'undefined') {
        Module.getGPIOState = getGPIOState;
        Module.setGPIOState = setGPIOState;
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

/**
 * Load the WebAssembly module
 */
async function loadModule() {
    state.Module = await createPlantWateringModule(window.Module);
    
    // Attach functions to the new Module object
    state.Module.getGPIOState = getGPIOState;
    state.Module.setGPIOState = setGPIOState;
    
    // Update global reference
    window.Module = state.Module;
    
    console.log('[APP] Functions re-attached after module creation');
    
    addLog('Module loaded successfully', 'info');
    
    state.Module._main();
}

/**
 * Setup event listeners
 */
function setupEventListeners() {
    const speedSlider = document.getElementById('speedSlider');
    if (speedSlider) {
        speedSlider.addEventListener('input', updateSpeed);
    }
    
    console.log('[APP] Event listeners setup complete');
}

/**
 * Initialize the application
 */
function init() {
    console.log('[APP] Initializing Plant Watering Simulator...');
    
    // Expose all functions to global scope FIRST
    exposeGlobalFunctions();
    
    // Initialize graph
    initGraph();
    
    // Setup event listeners
    setupEventListeners();
    
    // Load WASM module (simulation NOT started automatically)
    loadModule();
    
    console.log('[APP] Initialization complete');
}

// Initialize on page load
window.addEventListener('load', init);

export { init, loadModule };