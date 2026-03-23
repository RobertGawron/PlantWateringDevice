// ------------------------------------------------------------
// Type Definitions
// ------------------------------------------------------------

/**
 * Log level types matching the HTML filter checkboxes
 */
export type WateringLogLevel = 'info' | 'warning' | 'error' | 'debug-low' | 'debug-high';

/**
 * Filter checkbox IDs mapped from log levels
 */
type FilterId = 
    | 'filterInfo' 
    | 'filterWarning' 
    | 'filterError' 
    | 'filterDebugLow' 
    | 'filterDebugHigh';

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

/**
 * Maximum number of log entries to keep in the DOM
 */
const MAX_LOG_ENTRIES = 200;

/**
 * Log element ID in the DOM
 */
const LOG_ELEMENT_ID = 'log';

// ------------------------------------------------------------
// Helper Functions
// ------------------------------------------------------------

/**
 * Convert log type to filter checkbox ID
 * 
 * @example
 * getFilterId('debug-low')  // returns 'filterDebugLow'
 * getFilterId('info')       // returns 'filterInfo'
 * 
 * @param type - Log type like "debug-low", "info", etc.
 * @returns Filter ID like "filterDebugLow"
 */
function getFilterId(type: WateringLogLevel): FilterId {
    // Split by hyphen and capitalize each part
    // "debug-low" -> ["debug", "low"] -> ["Debug", "Low"] -> "DebugLow"
    const parts = type.split('-');
    const camelCase = parts
        .map((part) => part.charAt(0).toUpperCase() + part.slice(1).toLowerCase())
        .join('');
    return `filter${camelCase}` as FilterId;
}

/**
 * Check if a log level is currently enabled via filter checkbox
 * 
 * @param type - Log level to check
 * @returns true if the log level should be displayed
 */
function isWateringLogLevelEnabled(type: WateringLogLevel): boolean {
    const filterId = getFilterId(type);
    const filterCheckbox = document.getElementById(filterId) as HTMLInputElement | null;
    
    // If checkbox doesn't exist, default to showing the log
    if (!filterCheckbox) {
        return true;
    }
    
    return filterCheckbox.checked;
}

/**
 * Get the log container element
 * 
 * @returns The log element or null if not found
 */
function getLogElement(): HTMLElement | null {
    return document.getElementById(LOG_ELEMENT_ID);
}

/**
 * Format current time as a timestamp string
 * 
 * @returns Formatted timestamp like "14:32:05"
 */
function getTimestamp(): string {
    return new Date().toLocaleTimeString();
}

/**
 * Trim old log entries to stay under MAX_LOG_ENTRIES
 * 
 * @param logElement - The log container element
 */
function trimOldEntries(logElement: HTMLElement): void {
    while (logElement.children.length > MAX_LOG_ENTRIES) {
        const firstChild = logElement.firstChild;
        if (firstChild) {
            logElement.removeChild(firstChild);
        }
    }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
// These functions use the underscore naming convention because 
// that is the convention in C, and they are used in C code.
// Firmware is the main part of the project; simulation 
// is secondary. Therefore, the inconsistency will be in 
// TypeScript, not in the firmware. 
/**
 * Add a log entry to the system log
 * 
 * @example
 * addLog('Application started', 'info');
 * addLog('Button pressed', 'debug-low');
 * addLog('Connection failed', 'error');
 * 
 * @param message - Log message to display
 * @param type - Log type: 'info', 'warning', 'error', 'debug-low', 'debug-high'
 */
export function addLog(message: string, type: WateringLogLevel = 'info'): void {
    // Check if this log level is filtered out
    if (!isWateringLogLevelEnabled(type)) {
        return;
    }

    const log = getLogElement();

    if (!log) {
        console.warn('[LOGGING] Log element not found, message:', message);
        return;
    }

    // Create log entry element
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${getTimestamp()}] ${message}`;

    // Add to log and scroll to bottom
    log.appendChild(entry);
    log.scrollTop = log.scrollHeight;

    // Keep only last MAX_LOG_ENTRIES entries
    trimOldEntries(log);
}

/**
 * Clear all log entries
 */
export function clearLog(): void {
    const log = getLogElement();
    
    if (log) {
        log.innerHTML = '';
    }
    
    addLog('Log cleared', 'info');
}

/**
 * Add an info log entry
 * 
 * @param message - Log message
 */
export function WATERING_LOG_INFO(message: string): void {
    addLog(message, 'info');
}

/**
 * Add a warning log entry
 * 
 * @param message - Log message
 */
export function WATERING_LOG_WARNING(message: string): void {
    addLog(message, 'warning');
}

/**
 * Add an error log entry
 * 
 * @param message - Log message
 */
export function WATERING_LOG_ERROR(message: string): void {
    addLog(message, 'error');
}

/**
 * Add a debug-low log entry (high frequency debug messages)
 * 
 * @param message - Log message
 */
export function WATERING_LOG_DEBUG_LOW(message: string): void {
    addLog(message, 'debug-low');
}

/**
 * Add a debug-high log entry (important debug messages)
 * 
 * @param message - Log message
 */
export function WATERING_LOG_DEBUG_HIGH(message: string): void {
    addLog(message, 'debug-high');
}

/**
 * Log an error object with proper message extraction
 * 
 * @param context - Context description for the error
 * @param error - Error object or unknown error
 */
export function WATERING_LOG_ERRORObject(context: string, error: unknown): void {
    const errorMessage = error instanceof Error ? error.message : String(error);
    addLog(`${context}: ${errorMessage}`, 'error');
    console.error(`[LOGGING] ${context}:`, error);
}