import { state, gpioState } from './state.js';
import { addLog, clearLog } from './logging.js';
import { buttonDown, buttonUp, toggleSoil, getGPIOState, setGPIOState, debugGPIOState } from './gpio.js';
import { startSimulation, stopSimulation, toggleSimulation, updateSpeed, singleStep, runMinute, runHour } from './simulation.js';
import { initGraph, clearGraph } from './graph.js';

/**
 * Expose all functions to global scope
 */
function exposeGlobalFunctions() {
    // Logging
    window.addLog = addLog;
    window.clearLog = clearLog;
    
    // GPIO (for WASM)
    window.getGPIOState = getGPIOState;
    window.setGPIOState = setGPIOState;
    
    // Debug
    window.debugGPIO = debugGPIOState;
    window.gpioState = gpioState;
    
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
    
    console.log('[APP] Global functions exposed');
}

/**
 * Load the WebAssembly module
 */
async function loadModule() {
    addLog('Loading WebAssembly module...', 'info');
    
    try {
        if (typeof createPlantWateringModule !== 'function') {
            throw new Error('createPlantWateringModule not found');
        }
        
        state.Module = await createPlantWateringModule();
        
        addLog('Module loaded successfully', 'info');
        addLog('Firmware ready - click Start to begin simulation', 'info');

        // Start main() in background
        setTimeout(() => {
            state.Module._main();
        }, 100);
        
        // NOTE: Simulation is NOT started automatically
        // User must click "Start" button
        
    } catch (error) {
        addLog(`Failed to load module: ${error.message}`, 'error');
        console.error('[APP] Module load error:', error);
    }
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