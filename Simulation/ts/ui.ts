import { state } from './state';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

/**
 * Duration of one tick in seconds (20ms)
 */
const TICK_DURATION_SECONDS = 0.02;

/**
 * Seconds per minute
 */
const SECONDS_PER_MINUTE = 60;

/**
 * Seconds per hour
 */
const SECONDS_PER_HOUR = 3600;

/**
 * Maximum display value before wrapping (0-9)
 */
const MAX_DISPLAY_VALUE = 10;

/**
 * DOM element IDs
 */
const DOM_IDS = {
    SEVEN_SEGMENT: 'sevenSegment',
    TICK_COUNT: 'tickCount',
    RUNTIME: 'runtime',
    SPEED_SLIDER: 'speedSlider',
    SPEED_DISPLAY: 'speedDisplay',
    START_STOP_BUTTON: 'btnStartStop',
    LOG: 'log',
    BUTTON_PRESS: 'btnPress'
} as const;

/**
 * CSS classes used for UI state
 */
const CSS_CLASSES = {
    RUNNING: 'running',
    ACTIVE: 'active',
    DISABLED: 'disabled'
} as const;

// ------------------------------------------------------------
// Type Definitions
// ------------------------------------------------------------

/**
 * Runtime display format
 */
interface RuntimeDisplay {
    hours: number;
    minutes: number;
    seconds: number;
    formatted: string;
}

// ------------------------------------------------------------
// DOM Element Helpers
// ------------------------------------------------------------

/**
 * Safely get an element by ID with type assertion
 * 
 * @param id - Element ID
 * @returns The element or null if not found
 */
function getElement<T extends HTMLElement = HTMLElement>(id: string): T | null {
    return document.getElementById(id) as T | null;
}

/**
 * Safely set text content of an element by ID
 * 
 * @param id - Element ID
 * @param text - Text content to set
 * @returns true if element was found and updated
 */
function setElementText(id: string, text: string): boolean {
    const element = getElement(id);
    if (element) {
        element.textContent = text;
        return true;
    }
    return false;
}

/**
 * Safely add a CSS class to an element by ID
 * 
 * @param id - Element ID
 * @param className - CSS class to add
 * @returns true if element was found and updated
 */
function addElementClass(id: string, className: string): boolean {
    const element = getElement(id);
    if (element) {
        element.classList.add(className);
        return true;
    }
    return false;
}

/**
 * Safely remove a CSS class from an element by ID
 * 
 * @param id - Element ID
 * @param className - CSS class to remove
 * @returns true if element was found and updated
 */
function removeElementClass(id: string, className: string): boolean {
    const element = getElement(id);
    if (element) {
        element.classList.remove(className);
        return true;
    }
    return false;
}

/**
 * Safely toggle a CSS class on an element by ID
 * 
 * @param id - Element ID
 * @param className - CSS class to toggle
 * @param force - Force add (true) or remove (false)
 * @returns true if element was found and updated
 */
function toggleElementClass(id: string, className: string, force?: boolean): boolean {
    const element = getElement(id);
    if (element) {
        element.classList.toggle(className, force);
        return true;
    }
    return false;
}

// ------------------------------------------------------------
// Time Formatting
// ------------------------------------------------------------

/**
 * Calculate runtime from tick count
 * 
 * @param tickCount - Number of ticks
 * @returns Runtime breakdown with formatted string
 */
function calculateRuntime(tickCount: number): RuntimeDisplay {
    const totalSeconds = Math.floor(tickCount * TICK_DURATION_SECONDS);
    const hours = Math.floor(totalSeconds / SECONDS_PER_HOUR);
    const minutes = Math.floor((totalSeconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
    const seconds = totalSeconds % SECONDS_PER_MINUTE;

    let formatted: string;
    if (hours > 0) {
        formatted = `${hours}h ${minutes}m`;
    } else if (minutes > 0) {
        formatted = `${minutes}m ${seconds}s`;
    } else {
        formatted = `${seconds}s`;
    }

    return { hours, minutes, seconds, formatted };
}

/**
 * Format seconds into a human-readable time string
 * 
 * @param totalSeconds - Total seconds to format
 * @returns Formatted time string (e.g., "1h 30m 45s")
 */
export function formatTime(totalSeconds: number): string {
    const hours = Math.floor(totalSeconds / SECONDS_PER_HOUR);
    const minutes = Math.floor((totalSeconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
    const seconds = Math.floor(totalSeconds % SECONDS_PER_MINUTE);

    if (hours > 0) {
        return `${hours}h ${minutes}m ${seconds}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${seconds}s`;
    } else {
        return `${seconds}s`;
    }
}

/**
 * Format seconds into compact time string
 * 
 * @param totalSeconds - Total seconds to format
 * @returns Compact formatted time string (e.g., "1:30:45")
 */
export function formatTimeCompact(totalSeconds: number): string {
    const hours = Math.floor(totalSeconds / SECONDS_PER_HOUR);
    const minutes = Math.floor((totalSeconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
    const seconds = Math.floor(totalSeconds % SECONDS_PER_MINUTE);

    if (hours > 0) {
        return `${hours}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
    } else {
        return `${minutes}:${seconds.toString().padStart(2, '0')}`;
    }
}

// ------------------------------------------------------------
// Seven Segment Display
// ------------------------------------------------------------

/**
 * Update the seven-segment display with current value
 * 
 * Wraps the display value to 0-9 range and updates the DOM.
 */
export function updateSevenSegment(): void {
    // Wrap value to 0-9 range
    state.currentDisplayValue = state.currentDisplayValue % MAX_DISPLAY_VALUE;

    setElementText(DOM_IDS.SEVEN_SEGMENT, String(state.currentDisplayValue));
}

/**
 * Set the seven-segment display to a specific value
 * 
 * @param value - Value to display (will be wrapped to 0-9)
 */
export function setSevenSegment(value: number): void {
    state.currentDisplayValue = value % MAX_DISPLAY_VALUE;
    updateSevenSegment();
}

/**
 * Reset the seven-segment display to 0
 */
export function resetSevenSegment(): void {
    state.currentDisplayValue = 0;
    updateSevenSegment();
}

/**
 * Increment the seven-segment display value
 */
export function incrementSevenSegment(): void {
    state.currentDisplayValue++;
    updateSevenSegment();
}

// ------------------------------------------------------------
// Status Display
// ------------------------------------------------------------

/**
 * Update status counters and runtime display
 * 
 * Updates the tick count and runtime displays in the UI.
 */
export function updateStatus(): void {
    // Update tick count
    setElementText(DOM_IDS.TICK_COUNT, state.tickCount.toLocaleString());

    // Update runtime
    const runtime = calculateRuntime(state.tickCount);
    setElementText(DOM_IDS.RUNTIME, runtime.formatted);
}

/**
 * Reset status displays to initial state
 */
export function resetStatus(): void {
    setElementText(DOM_IDS.TICK_COUNT, '0');
    setElementText(DOM_IDS.RUNTIME, '0s');
}

/**
 * Get current runtime display values
 * 
 * @returns Runtime breakdown with formatted string
 */
export function getRuntime(): RuntimeDisplay {
    return calculateRuntime(state.tickCount);
}

// ------------------------------------------------------------
// Button States
// ------------------------------------------------------------

/**
 * Update the Start/Stop button state
 * 
 * @param isRunning - Whether the simulation is running
 */
export function updateStartStopButton(isRunning: boolean): void {
    const btn = getElement<HTMLButtonElement>(DOM_IDS.START_STOP_BUTTON);
    if (btn) {
        btn.textContent = isRunning ? 'Stop' : 'Start';
        btn.classList.toggle(CSS_CLASSES.RUNNING, isRunning);
    }
}

/**
 * Set button enabled/disabled state
 * 
 * @param buttonId - Button element ID
 * @param enabled - Whether the button should be enabled
 */
export function setButtonEnabled(buttonId: string, enabled: boolean): void {
    const btn = getElement<HTMLButtonElement>(buttonId);
    if (btn) {
        btn.disabled = !enabled;
        btn.classList.toggle(CSS_CLASSES.DISABLED, !enabled);
    }
}

/**
 * Show button as active (pressed)
 * 
 * @param buttonId - Button element ID
 */
export function setButtonActive(buttonId: string): void {
    addElementClass(buttonId, CSS_CLASSES.ACTIVE);
}

/**
 * Show button as inactive (not pressed)
 * 
 * @param buttonId - Button element ID
 */
export function setButtonInactive(buttonId: string): void {
    removeElementClass(buttonId, CSS_CLASSES.ACTIVE);
}

// ------------------------------------------------------------
// Speed Display
// ------------------------------------------------------------

/**
 * Update the speed display
 * 
 * @param speed - Current speed multiplier
 */
export function updateSpeedDisplay(speed: number): void {
    setElementText(DOM_IDS.SPEED_DISPLAY, `${speed}x`);
}

/**
 * Get the current speed slider value
 * 
 * @returns Current speed value or 1 if not available
 */
export function getSpeedSliderValue(): number {
    const slider = getElement<HTMLInputElement>(DOM_IDS.SPEED_SLIDER);
    if (slider) {
        return parseInt(slider.value, 10) || 1;
    }
    return 1;
}

/**
 * Set the speed slider value
 * 
 * @param speed - Speed value to set
 */
export function setSpeedSliderValue(speed: number): void {
    const slider = getElement<HTMLInputElement>(DOM_IDS.SPEED_SLIDER);
    if (slider) {
        slider.value = String(speed);
    }
    updateSpeedDisplay(speed);
}

// ------------------------------------------------------------
// Visual Indicators
// ------------------------------------------------------------

/**
 * Indicator state type
 */
export type IndicatorState = 'on' | 'off' | 'error' | 'warning';

/**
 * Set visual indicator state for an element
 * 
 * @param elementId - Element ID
 * @param indicatorState - State to set
 */
export function setIndicatorState(elementId: string, indicatorState: IndicatorState): void {
    const element = getElement(elementId);
    if (element) {
        // Remove all state classes
        element.classList.remove('indicator-on', 'indicator-off', 'indicator-error', 'indicator-warning');
        // Add new state class
        element.classList.add(`indicator-${indicatorState}`);
    }
}

/**
 * Flash an element to draw attention
 * 
 * @param elementId - Element ID
 * @param duration - Flash duration in milliseconds (default 200)
 */
export function flashElement(elementId: string, duration: number = 200): void {
    const element = getElement(elementId);
    if (element) {
        element.classList.add('flash');
        setTimeout(() => {
            element.classList.remove('flash');
        }, duration);
    }
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

/**
 * Initialize all UI elements to default state
 */
export function initializeUI(): void {
    resetSevenSegment();
    resetStatus();
    updateStartStopButton(false);
    updateSpeedDisplay(1);
}

/**
 * Refresh all UI elements from current state
 * 
 * Useful after state changes or module reloads.
 */
export function refreshUI(): void {
    updateSevenSegment();
    updateStatus();
    updateStartStopButton(state.isRunning);
    updateSpeedDisplay(getSpeedSliderValue());
}

// ------------------------------------------------------------
// Exports
// ------------------------------------------------------------

export {
    DOM_IDS,
    CSS_CLASSES,
    TICK_DURATION_SECONDS,
    type RuntimeDisplay
};