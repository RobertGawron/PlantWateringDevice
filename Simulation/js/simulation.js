import { state, gpioState } from './state.js';
import { addLog } from './logging.js';
import { updateDisplay, updateStatus } from './ui.js';
import { addGraphData } from './graph.js';

/**
 * Start the simulation loop
 */
export function startSimulation() {
    addLog('Simulation started', 'info');
    state.isRunning = true;
    updateSpeed();
}

/**
 * Stop the simulation loop
 */
export function stopSimulation() {
    state.isRunning = false;
    if (state.intervalId) {
        clearInterval(state.intervalId);
        state.intervalId = null;
    }
    addLog('Simulation stopped', 'info');
}

/**
 * Update simulation speed based on slider
 */
export function updateSpeed() {
    if (state.intervalId) {
        clearInterval(state.intervalId);
    }

    const speed = parseInt(document.getElementById('speedSlider').value);
    const interval = 20 / speed; // Base 20ms divided by speed multiplier

    document.getElementById('speedDisplay').textContent = `${speed}x`;

    if (state.isRunning) {
        state.intervalId = setInterval(tick, interval);
    }
}

/**
 * Execute a single simulation tick
 */
export function tick() {
    if (!state.Module) return;

    state.Module._advanceTick();
    state.tickCount++;
    
    updateDisplay();
    updateStatus();
    
    // Add data to graph (every 50 ticks to reduce load)
    if (state.tickCount % 50 === 0) {
        const timestamp = state.tickCount * 0.02; // Convert to seconds
        addGraphData(gpioState[2], gpioState[1], gpioState[0], timestamp);
    }
}

/**
 * Execute a single step
 */
export function singleStep() {
    tick();
    addLog(`Single step executed (tick ${state.tickCount})`, 'debug-high');
}

/**
 * Fast-forward 1 minute (3000 ticks)
 */
export function runMinute() {
    addLog('Fast-forwarding 1 minute (3000 ticks)...', 'warning');
    for (let i = 0; i < 3000; i++) {
        tick();
    }
    addLog('1 minute completed', 'info');
}

/**
 * Fast-forward 1 hour (180000 ticks)
 */
export function runHour() {
    addLog('Fast-forwarding 1 hour (180000 ticks)...', 'warning');
    for (let i = 0; i < 180000; i++) {
        tick();
    }
    addLog('1 hour completed', 'info');
}