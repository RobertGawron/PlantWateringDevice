import { state } from './state.js';
import { updateDisplayFromGPIO } from './gpio.js';

/**
 * Update all display elements
 */
export function updateDisplay() {
    updateDisplayFromGPIO();
}

/**
 * Update status counters and runtime display
 */
export function updateStatus() {
    document.getElementById('tickCount').textContent = state.tickCount.toLocaleString();
    
    const seconds = Math.floor(state.tickCount * 0.02);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    
    let runtime = '';
    if (hours > 0) {
        runtime = `${hours}h ${minutes % 60}m`;
    } else if (minutes > 0) {
        runtime = `${minutes}m ${seconds % 60}s`;
    } else {
        runtime = `${seconds}s`;
    }
    
    document.getElementById('runtime').textContent = runtime;
}