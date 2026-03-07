// js/app.js - Main application entry point

import { state, gpioState } from './state.js';
import { addLog, clearLog } from './logging.js';
import { buttonDown, buttonUp, toggleSoil, getGPIOState, setGPIOState } from './gpio.js';
import { startSimulation, stopSimulation, updateSpeed, singleStep, runMinute, runHour } from './simulation.js';
import { initGraph, clearGraph } from './graph.js';

/**
 * Expose all functions to global scope for HTML onclick handlers and WASM module
 */
function exposeGlobalFunctions() {
    // Logging functions (needed by C code MAIN_THREAD_EM_ASM)
    window.addLog = addLog;
    window.clearLog = clearLog;
    
    // GPIO functions (needed by WASM module)
    window.getGPIOState = getGPIOState;
    window.setGPIOState = setGPIOState;
    
    // Button and control functions (needed by HTML onclick handlers)
    window.buttonDown = buttonDown;
    window.buttonUp = buttonUp;
    window.toggleSoil = toggleSoil;
    
    // Simulation control functions (needed by HTML onclick handlers)
    window.singleStep = singleStep;
    window.runMinute = runMinute;
    window.runHour = runHour;
    
    // Graph functions (needed by HTML onclick handlers)
    window.clearGraph = clearGraph;
    
    console.log('Global functions exposed to window object');
}

/**
 * Load the WebAssembly module
 */
async function loadModule() {
    addLog('Loading WebAssembly module...', 'info');
    
    try {
        // Check if createPlantWateringModule exists
        if (typeof createPlantWateringModule !== 'function') {
            throw new Error('createPlantWateringModule not found. Is plant-watering.js loaded?');
        }
        
        // Load the WASM module
        state.Module = await createPlantWateringModule();
        
        addLog('Module loaded successfully', 'info');
        addLog('Starting firmware...', 'info');
        
        // Verify addLog is available globally before calling main()
        if (typeof window.addLog !== 'function') {
            console.error('addLog not exposed to window!');
            addLog('Warning: addLog may not be available to C code', 'warning');
        }
        
        // Start main() in background - delay ensures everything is ready
        setTimeout(() => {
            console.log('Calling Module._main()');
            try {
                state.Module._main();
            } catch (error) {
                console.error('Error in Module._main():', error);
                addLog(`Firmware error: ${error.message}`, 'error');
            }
        }, 500);

        // Start the simulation tick generator
        setTimeout(() => {
            startSimulation();
        }, 600);
        
    } catch (error) {
        const errorMsg = `Failed to load module: ${error.message}`;
        addLog(errorMsg, 'error');
        console.error('Module load error:', error);
    }
}

/**
 * Setup event listeners
 */
function setupEventListeners() {
    // Speed slider control
    const speedSlider = document.getElementById('speedSlider');
    if (speedSlider) {
        speedSlider.addEventListener('input', updateSpeed);
    }
    
    // Graph time window selector
    const graphTimeWindow = document.getElementById('graphTimeWindow');
    if (graphTimeWindow) {
        graphTimeWindow.addEventListener('change', () => {
            // Graph will update on next data point
            console.log('Time window changed to:', graphTimeWindow.value);
        });
    }
    
    // Keyboard shortcuts (optional)
    document.addEventListener('keydown', (event) => {
        switch (event.key) {
            case ' ':
                // Spacebar - single step
                if (!event.repeat) {
                    singleStep();
                }
                event.preventDefault();
                break;
            case 'b':
            case 'B':
                // B key - simulate button press
                buttonDown();
                break;
            case 's':
            case 'S':
                // S key - toggle soil
                if (!event.repeat) {
                    toggleSoil();
                }
                break;
        }
    });
    
    document.addEventListener('keyup', (event) => {
        switch (event.key) {
            case 'b':
            case 'B':
                // B key released - simulate button release
                buttonUp();
                break;
        }
    });
    
    // Handle page visibility changes (pause when tab is hidden)
    document.addEventListener('visibilitychange', () => {
        if (document.hidden) {
            // Page is hidden - could pause simulation to save resources
            console.log('Page hidden');
        } else {
            // Page is visible again
            console.log('Page visible');
        }
    });
    
    // Handle window unload (cleanup)
    window.addEventListener('beforeunload', () => {
        stopSimulation();
    });
    
    console.log('Event listeners setup complete');
}

/**
 * Initialize the application
 */
function init() {
    console.log('Initializing Plant Watering Simulator...');
    
    // FIRST: Expose all functions to global scope
    // This must happen before WASM module tries to call addLog
    exposeGlobalFunctions();
    
    // Initialize the logic analyzer graph
    initGraph();
    
    // Setup all event listeners
    setupEventListeners();
    
    // Load the WASM module (async)
    loadModule();
    
    console.log('Initialization complete');
}

/**
 * Alternative initialization if DOM is already loaded
 */
function onReady(callback) {
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', callback);
    } else {
        callback();
    }
}

// Initialize on page load
window.addEventListener('load', init);

// Also expose init for manual calling if needed
window.initApp = init;

// Export for potential use by other modules
export { init, loadModule };