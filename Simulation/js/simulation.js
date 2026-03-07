import { state, gpioState } from './state.js';
import { addLog } from './logging.js';
import {  updateStatus } from './ui.js';
import { addGraphDataFromGPIO } from './graph.js';

/**
 * Start the simulation loop
 */
export function startSimulation() {
    if (state.isRunning) return;

    addLog('Simulation started', 'info');
    state.isRunning = true;
    updateStartStopButton();
    updateSpeed();
}

/**
 * Stop the simulation loop
 */
export function stopSimulation() {
    if (!state.isRunning) return;

    state.isRunning = false;
    if (state.intervalId) {
        clearInterval(state.intervalId);
        state.intervalId = null;
    }
    updateStartStopButton();
    addLog('Simulation stopped', 'info');
}

/**
 * Toggle simulation start/stop
 */
export function toggleSimulation() {
    if (state.isRunning) {
        stopSimulation();
    } else {
        startSimulation();
    }
}

/**
 * Update the Start/Stop button text
 */
function updateStartStopButton() {
    const btn = document.getElementById('btnStartStop');
    if (btn) {
        btn.textContent = state.isRunning ? 'Stop' : 'Start';
        btn.classList.toggle('running', state.isRunning);
    }
}

/**
 * Update simulation speed based on slider
 */
export function updateSpeed() {
    if (state.intervalId) {
        clearInterval(state.intervalId);
    }

    const speed = parseInt(document.getElementById('speedSlider').value);
    const interval = 20 / speed;

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

    updateStatus();

    // Update graph every tick
    const timestamp = state.tickCount * 0.02;
    addGraphDataFromGPIO({ ...gpioState }, timestamp);
}

/**
 * Execute a single step (works even when simulation is stopped)
 */
export function singleStep() {
    if (!state.Module) {
        addLog('Module not loaded', 'error');
        return;
    }

    tick();
    addLog(`Single step executed (tick ${state.tickCount})`, 'debug-high');
}

/**
 * Fast-forward 1 minute (3000 ticks)
 */
export function runMinute() {
    if (!state.Module) {
        addLog('Module not loaded', 'error');
        return;
    }

    addLog('Running 1 minute (3000 ticks)...', 'warning');

    for (let i = 0; i < 3000; i++) {
        state.Module._advanceTick();
        state.tickCount++;

        const timestamp = state.tickCount * 0.02;
        addGraphDataFromGPIO({ ...gpioState }, timestamp);
    }

    updateStatus();
    addLog('1 minute completed', 'info');
}

/**
 * Fast-forward 1 hour (180000 ticks)
 */
export function runHour() {
    if (!state.Module) {
        addLog('Module not loaded', 'error');
        return;
    }

    addLog('Running 1 hour (180000 ticks)...', 'warning');

    for (let i = 0; i < 180000; i++) {
        state.Module._advanceTick();
        state.tickCount++;

        // Update graph every 100 ticks during fast-forward
        if (i % 100 === 0) {
            const timestamp = state.tickCount * 0.02;
            addGraphDataFromGPIO({ ...gpioState }, timestamp);
        }
    }

    updateStatus();
    addLog('1 hour completed', 'info');
}