export let Module = null;

export async function loadModule() {
    addLog('Loading WebAssembly module...', 'info');
    
    try {
        if (typeof createPlantWateringModule !== 'function') {
            throw new Error('createPlantWateringModule not found');
        }
        
        // Use the global Module object configured in HTML
        state.Module = await createPlantWateringModule(window.Module);
        
        // Verify the functions are there
        console.log('[APP] Module.getGPIOState type:', typeof state.Module.getGPIOState);
        console.log('[APP] Module.setGPIOState type:', typeof state.Module.setGPIOState);
        
        addLog('Module loaded successfully', 'info');
        addLog('Firmware ready - click Start to begin simulation', 'info');

        // Start main() in background
        setTimeout(() => {
            state.Module._main();
        }, 100);
        
    } catch (error) {
        addLog(`Failed to load module: ${error.message}`, 'error');
        console.error('[APP] Module load error:', error);
    }
}