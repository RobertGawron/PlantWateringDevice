import { state, gpioState, GPIOPin } from './state';
import { addLog } from './logging';
import { updateStatus } from './ui';
import { addGraphDataFromGPIO } from './graph';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

const TICK_DURATION_SECONDS = 0.02;
const BASE_INTERVAL_MS = 20;
const TICKS_PER_MINUTE = 3000;
const TICKS_PER_HOUR = 180000;
const FAST_FORWARD_GRAPH_INTERVAL = 100;
const DEFAULT_SPEED = 1;

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

// ------------------------------------------------------------
// Core Tick Function
// ------------------------------------------------------------

/**
 * Execute one firmware tick.
 * 
 * In the new architecture, _main() executes one complete iteration
 * and returns immediately (no while loop, no delays).
 */
export function tick(): void {
    if (!state.Module) {
        return;
    }

    // Call main() - it executes one tick and returns
    state.Module._main();
    
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
// Fast-Forward
// ------------------------------------------------------------

let fastForwardInProgress = false;

export function isFastForwardRunning(): boolean {
    return fastForwardInProgress;
}

/**
 * Fast-forward simulation by executing ticks as fast as possible.
 * 
 * Since _main() now returns immediately, this runs BLAZINGLY fast.
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

    if (fastForwardInProgress) {
        addLog('Fast-forward already in progress', 'warning');
        return;
    }

    fastForwardInProgress = true;
    addLog(`Running ${description} (${totalTicks.toLocaleString()} ticks)...`, 'info');

    const startTime = performance.now();

    try {
        for (let i = 0; i < totalTicks; i++) {
            state.Module._main();  // ← Runs instantly, no delay
            state.tickCount++;

            if (i % graphInterval === 0) {
                recordGraphData();
            }
        }

        recordGraphData();
        updateStatus();

        const elapsed = ((performance.now() - startTime) / 1000).toFixed(2);
        addLog(`${description} completed in ${elapsed}s`, 'info');

    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        addLog(`Fast-forward error: ${message}`, 'error');
        console.error('[FAST-FORWARD] Error:', error);

    } finally {
        fastForwardInProgress = false;
    }
}

export function runMinute(): void {
    fastForward(TICKS_PER_MINUTE, 1, '1 minute');
}

export function runHour(): void {
    fastForward(TICKS_PER_HOUR, FAST_FORWARD_GRAPH_INTERVAL, '1 hour');
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