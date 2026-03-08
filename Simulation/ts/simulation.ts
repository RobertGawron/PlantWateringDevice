import { state, gpioState, GPIOPin } from './state';
import { addLog } from './logging';
import { updateStatus } from './ui';
import { addGraphDataFromGPIO } from './graph';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

/** Duration of one simulation tick in seconds (20ms) */
const TICK_DURATION_SECONDS = 0.02;

/** Base interval in milliseconds at 1x speed */
const BASE_INTERVAL_MS = 20;

/** Ticks per minute: 60s / 0.02s = 3000 ticks */
const TICKS_PER_MINUTE = 3000;

/** Ticks per hour: 3600s / 0.02s = 180000 ticks */
const TICKS_PER_HOUR = 180000;

/** 
 * Graph recording interval during fast-forward.
 * Lower values = more resolution but more memory usage.
 */
const FAST_FORWARD_GRAPH_INTERVAL = 100;

/** Default speed multiplier when slider is unavailable */
const DEFAULT_SPEED = 1;

/** 
 * Yield interval for fast-forward.
 * Lower = more responsive but slower.
 * Higher = faster but may miss GPIO updates.
 */
const FAST_FORWARD_YIELD_INTERVAL = 1;

/** DOM element IDs used by this module */
const DOM_IDS = {
    START_STOP_BUTTON: 'btnStartStop',
    SPEED_SLIDER: 'speedSlider',
    SPEED_DISPLAY: 'speedDisplay'
} as const;

// ------------------------------------------------------------
// DOM Helpers
// ------------------------------------------------------------

function getElement<T extends HTMLElement>(id: string): T | null {
    return document.getElementById(id) as T | null;
}

function getCurrentSpeed(): number {
    const slider = getElement<HTMLInputElement>(DOM_IDS.SPEED_SLIDER);
    return slider ? (parseInt(slider.value, 10) || DEFAULT_SPEED) : DEFAULT_SPEED;
}

function updateStartStopButton(): void {
    const button = getElement<HTMLButtonElement>(DOM_IDS.START_STOP_BUTTON);
    if (button) {
        button.textContent = state.isRunning ? 'Stop' : 'Start';
        button.classList.toggle('running', state.isRunning);
    }
}

function updateSpeedDisplay(speed: number): void {
    const display = getElement(DOM_IDS.SPEED_DISPLAY);
    if (display) {
        display.textContent = `${speed}x`;
    }
}

// ------------------------------------------------------------
// Internal Helpers
// ------------------------------------------------------------

function clearSimulationInterval(): void {
    if (state.intervalId !== null) {
        clearInterval(state.intervalId);
        state.intervalId = null;
    }
}

function recordGraphData(): void {
    const timestamp = state.tickCount * TICK_DURATION_SECONDS;
    addGraphDataFromGPIO({ ...gpioState }, timestamp);
}

function isModuleReady(): boolean {
    return state.Module !== null;
}

/**
 * Yield to the browser event loop.
 * Allows MAIN_THREAD_EM_ASM callbacks to execute.
 */
function yieldToEventLoop(): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, 0));
}

// ------------------------------------------------------------
// Core Tick Function
// ------------------------------------------------------------

export function tick(): void {
    if (!state.Module) {
        return;
    }

    state.Module._advanceTick();
    state.tickCount++;

    updateStatus();
    recordGraphData();
}

// ------------------------------------------------------------
// Simulation Control
// ------------------------------------------------------------

export function startSimulation(): void {
    if (state.isRunning) {
        return;
    }

    if (!isModuleReady()) {
        addLog('Cannot start: Module not loaded', 'error');
        return;
    }

    state.isRunning = true;
    updateStartStopButton();
    updateSpeed();

    addLog('Simulation started', 'info');
}

export function stopSimulation(): void {
    if (!state.isRunning) {
        return;
    }

    state.isRunning = false;
    clearSimulationInterval();
    updateStartStopButton();

    addLog('Simulation stopped', 'info');
}

export function toggleSimulation(): void {
    if (state.isRunning) {
        stopSimulation();
    } else {
        startSimulation();
    }
}

export function updateSpeed(): void {
    clearSimulationInterval();

    const speed = getCurrentSpeed();
    const interval = BASE_INTERVAL_MS / speed;

    updateSpeedDisplay(speed);

    if (state.isRunning) {
        state.intervalId = window.setInterval(tick, interval);
    }
}

// ------------------------------------------------------------
// Manual Stepping
// ------------------------------------------------------------

export function singleStep(): void {
    if (!isModuleReady()) {
        addLog('Module not loaded', 'error');
        return;
    }

    tick();
    addLog(`Single step executed (tick ${state.tickCount.toLocaleString()})`, 'debug-high');
}

// ------------------------------------------------------------
// Fast-Forward (Async)
// ------------------------------------------------------------

/** Flag to track if fast-forward is in progress */
let fastForwardInProgress = false;

/**
 * Check if fast-forward is currently running.
 */
export function isFastForwardRunning(): boolean {
    return fastForwardInProgress;
}

/**
 * Execute multiple ticks asynchronously.
 * 
 * Yields to the event loop periodically to allow:
 * - MAIN_THREAD_EM_ASM callbacks to execute
 * - UI updates
 * - User interaction (cancel)
 */
async function fastForward(
    totalTicks: number,
    graphInterval: number,
    description: string
): Promise<void> {
    if (!state.Module) {
        addLog('Module not loaded', 'error');
        return;
    }

    if (fastForwardInProgress) {
        addLog('Fast-forward already in progress', 'warning');
        return;
    }

    fastForwardInProgress = true;

    console.log('[FAST-FORWARD] Starting:', description);
    console.log('[FAST-FORWARD] Soil sensor state:', gpioState[GPIOPin.SOIL_SENSOR]);

    addLog(`Running ${description} (${totalTicks.toLocaleString()} ticks)...`, 'info');

    const startTime = performance.now();
    let lastRecordedTick = -graphInterval;
    let lastProgressUpdate = 0;

    try {
        for (let i = 0; i < totalTicks; i++) {
            state.Module._advanceTick();
            state.tickCount++;

            // Record graph data at specified interval
            if (graphInterval === 1 || i - lastRecordedTick >= graphInterval) {
                recordGraphData();
                lastRecordedTick = i;
            }

            // Yield to event loop for WASM callbacks
            if (i % FAST_FORWARD_YIELD_INTERVAL === 0) {
                await yieldToEventLoop();
            }

            // Update progress every 10%
            const progress = Math.floor((i / totalTicks) * 10);
            if (progress > lastProgressUpdate) {
                lastProgressUpdate = progress;
                updateStatus();
            }
        }

        // Ensure final state is recorded
        if (totalTicks - 1 !== lastRecordedTick) {
            recordGraphData();
        }

        const elapsed = ((performance.now() - startTime) / 1000).toFixed(2);
        updateStatus();
        addLog(`${description} completed in ${elapsed}s`, 'info');

    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        addLog(`Fast-forward error: ${message}`, 'error');
        console.error('[FAST-FORWARD] Error:', error);

    } finally {
        fastForwardInProgress = false;
    }
}

/**
 * Fast-forward simulation by 1 minute (3000 ticks).
 */
export async function runMinute(): Promise<void> {
    await fastForward(TICKS_PER_MINUTE, 1, '1 minute');
}

/**
 * Fast-forward simulation by 1 hour (180000 ticks).
 */
export async function runHour(): Promise<void> {
    await fastForward(TICKS_PER_HOUR, FAST_FORWARD_GRAPH_INTERVAL, '1 hour');
}

// ------------------------------------------------------------
// State Queries
// ------------------------------------------------------------

export function isRunning(): boolean {
    return state.isRunning;
}

export function getTickCount(): number {
    return state.tickCount;
}

export function getSimulatedTimeSeconds(): number {
    return state.tickCount * TICK_DURATION_SECONDS;
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------

export function resetSimulationState(): void {
    stopSimulation();

    state.tickCount = 0;
    state.currentDisplayValue = 0;

    updateStatus();
    addLog('Simulation state reset', 'info');
}