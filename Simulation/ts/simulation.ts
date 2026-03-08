import { state, gpioState } from './state';
import { addLog } from './logging';
import { updateStatus } from './ui';
import { addGraphDataFromGPIO } from './graph';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

/**
 * Duration of one tick in seconds (20ms)
 */
const TICK_DURATION_SECONDS = 0.02;

/**
 * Base interval in milliseconds (20ms per tick at 1x speed)
 */
const BASE_INTERVAL_MS = 20;

/**
 * Number of ticks in one minute (3000 ticks = 60 seconds)
 */
const TICKS_PER_MINUTE = 3000;

/**
 * Number of ticks in one hour (180000 ticks = 3600 seconds)
 */
const TICKS_PER_HOUR = 180000;

/**
 * Graph update interval during fast-forward (every N ticks)
 */
const FAST_FORWARD_GRAPH_INTERVAL = 100;

/**
 * Default simulation speed multiplier
 */
const DEFAULT_SPEED = 1;

/**
 * DOM element IDs
 */
const DOM_IDS = {
    START_STOP_BUTTON: 'btnStartStop',
    SPEED_SLIDER: 'speedSlider',
    SPEED_DISPLAY: 'speedDisplay'
} as const;

// ------------------------------------------------------------
// Helper Functions
// ------------------------------------------------------------

/**
 * Get the speed slider element
 */
function getSpeedSlider(): HTMLInputElement | null {
    return document.getElementById(DOM_IDS.SPEED_SLIDER) as HTMLInputElement | null;
}

/**
 * Get the speed display element
 */
function getSpeedDisplay(): HTMLElement | null {
    return document.getElementById(DOM_IDS.SPEED_DISPLAY);
}

/**
 * Get the start/stop button element
 */
function getStartStopButton(): HTMLButtonElement | null {
    return document.getElementById(DOM_IDS.START_STOP_BUTTON) as HTMLButtonElement | null;
}

/**
 * Get current speed value from slider
 */
function getCurrentSpeed(): number {
    const slider = getSpeedSlider();
    if (!slider) {
        return DEFAULT_SPEED;
    }
    return parseInt(slider.value, 10) || DEFAULT_SPEED;
}

/**
 * Calculate the current simulation timestamp in seconds
 */
function getCurrentTimestamp(): number {
    return state.tickCount * TICK_DURATION_SECONDS;
}

/**
 * Check if the WASM module is loaded
 */
function isModuleLoaded(): boolean {
    return state.Module !== null;
}

/**
 * Update the Start/Stop button text and state
 */
function updateStartStopButton(): void {
    const btn = getStartStopButton();
    if (btn) {
        btn.textContent = state.isRunning ? 'Stop' : 'Start';
        btn.classList.toggle('running', state.isRunning);
    }
}

/**
 * Clear the current interval if one exists
 */
function clearCurrentInterval(): void {
    if (state.intervalId !== null) {
        clearInterval(state.intervalId);
        state.intervalId = null;
    }
}

/**
 * Record current GPIO state to graph
 */
function recordGraphData(): void {
    const timestamp = getCurrentTimestamp();
    addGraphDataFromGPIO({ ...gpioState }, timestamp);
}

// ------------------------------------------------------------
// Core Tick Function
// ------------------------------------------------------------

/**
 * Execute a single simulation tick
 * 
 * This advances the firmware by one tick (20ms simulated time),
 * updates the UI, and records data to the graph.
 */
export function tick(): void {
    if (!state.Module) {
        return;
    }

    // Advance the firmware simulation
    state.Module._advanceTick();
    state.tickCount++;

    // Update UI
    updateStatus();

    // Record to graph
    recordGraphData();
}

// ------------------------------------------------------------
// Simulation Control
// ------------------------------------------------------------

/**
 * Start the simulation loop
 * 
 * Begins continuous execution of ticks at the current speed setting.
 */
export function startSimulation(): void {
    if (state.isRunning) {
        return;
    }

    if (!isModuleLoaded()) {
        addLog('Cannot start: Module not loaded', 'error');
        return;
    }

    addLog('Simulation started', 'info');
    state.isRunning = true;
    updateStartStopButton();
    updateSpeed();
}

/**
 * Stop the simulation loop
 * 
 * Pauses continuous execution. The simulation can be resumed
 * with startSimulation() or stepped manually with singleStep().
 */
export function stopSimulation(): void {
    if (!state.isRunning) {
        return;
    }

    state.isRunning = false;
    clearCurrentInterval();
    updateStartStopButton();
    addLog('Simulation stopped', 'info');
}

/**
 * Toggle simulation between running and stopped states
 */
export function toggleSimulation(): void {
    if (state.isRunning) {
        stopSimulation();
    } else {
        startSimulation();
    }
}

/**
 * Update simulation speed based on slider value
 * 
 * Recalculates the tick interval and restarts the simulation
 * loop if currently running.
 */
export function updateSpeed(): void {
    // Clear existing interval
    clearCurrentInterval();

    // Get speed from slider
    const speed = getCurrentSpeed();
    const interval = BASE_INTERVAL_MS / speed;

    // Update speed display
    const speedDisplay = getSpeedDisplay();
    if (speedDisplay) {
        speedDisplay.textContent = `${speed}x`;
    }

    // Restart interval if simulation is running
    if (state.isRunning) {
        state.intervalId = window.setInterval(tick, interval);
    }
}

// ------------------------------------------------------------
// Manual Stepping
// ------------------------------------------------------------

/**
 * Execute a single step (works even when simulation is stopped)
 * 
 * Useful for debugging or precise control of the simulation.
 */
export function singleStep(): void {
    if (!isModuleLoaded()) {
        addLog('Module not loaded', 'error');
        return;
    }

    tick();
    addLog(`Single step executed (tick ${state.tickCount.toLocaleString()})`, 'debug-high');
}

// ------------------------------------------------------------
// Fast-Forward Functions
// ------------------------------------------------------------

/**
 * Fast-forward simulation by a specified number of ticks
 * 
 * @param totalTicks - Number of ticks to execute
 * @param graphInterval - Record to graph every N ticks (0 = every tick)
 * @param description - Description for log messages
 */
function fastForward(
    totalTicks: number,
    graphInterval: number,
    description: string
): void {
    if (!state.Module) {
        addLog('Module not loaded', 'error');
        return;
    }

    addLog(`Running ${description} (${totalTicks.toLocaleString()} ticks)...`, 'warning');

    const recordEveryTick = graphInterval === 0;

    for (let i = 0; i < totalTicks; i++) {
        state.Module._advanceTick();
        state.tickCount++;

        // Record to graph at specified interval
        if (recordEveryTick || i % graphInterval === 0) {
            recordGraphData();
        }
    }

    // Final graph update to ensure last state is recorded
    recordGraphData();

    updateStatus();
    addLog(`${description} completed`, 'info');
}

/**
 * Fast-forward 1 minute (3000 ticks)
 * 
 * Records every tick to the graph for full resolution.
 */
export function runMinute(): void {
    fastForward(TICKS_PER_MINUTE, 0, '1 minute');
}

/**
 * Fast-forward 1 hour (180000 ticks)
 * 
 * Records every 100 ticks to the graph to reduce memory usage.
 */
export function runHour(): void {
    fastForward(TICKS_PER_HOUR, FAST_FORWARD_GRAPH_INTERVAL, '1 hour');
}

/**
 * Fast-forward by a custom duration
 * 
 * @param seconds - Number of seconds to fast-forward
 * @param graphInterval - Record to graph every N ticks (default: 1)
 */
export function runSeconds(seconds: number, graphInterval: number = 1): void {
    const ticks = Math.floor(seconds / TICK_DURATION_SECONDS);
    fastForward(ticks, graphInterval, `${seconds} seconds`);
}

// ------------------------------------------------------------
// State Queries
// ------------------------------------------------------------

/**
 * Check if simulation is currently running
 */
export function isRunning(): boolean {
    return state.isRunning;
}

/**
 * Get current tick count
 */
export function getTickCount(): number {
    return state.tickCount;
}

/**
 * Get current simulated time in seconds
 */
export function getSimulatedTime(): number {
    return getCurrentTimestamp();
}

/**
 * Get current simulated time as formatted string
 */
export function getFormattedSimulatedTime(): string {
    const totalSeconds = getCurrentTimestamp();
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = Math.floor(totalSeconds % 60);

    if (hours > 0) {
        return `${hours}h ${minutes}m ${seconds}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${seconds}s`;
    } else {
        return `${seconds}s`;
    }
}

/**
 * Reset simulation state (tick count, etc.)
 * 
 * Note: This does NOT reset the WASM module state.
 * Use with caution.
 */
export function resetSimulationState(): void {
    stopSimulation();
    state.tickCount = 0;
    state.currentDisplayValue = 0;
    updateStatus();
    addLog('Simulation state reset', 'info');
}