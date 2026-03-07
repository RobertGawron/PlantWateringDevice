// js/logging.js - Logging system with filtering

/**
 * Convert log type to filter checkbox ID
 * @param {string} type - Log type like "debug-low", "info", etc.
 * @returns {string} Filter ID like "filterDebugLow"
 */
function getFilterId(type) {
    // Split by hyphen and capitalize each part
    // "debug-low" -> ["debug", "low"] -> ["Debug", "Low"] -> "DebugLow"
    const parts = type.split('-');
    const camelCase = parts
        .map(part => part.charAt(0).toUpperCase() + part.slice(1).toLowerCase())
        .join('');
    return `filter${camelCase}`;
}

/**
 * Add a log entry to the system log
 * @param {string} message - Log message
 * @param {string} type - Log type: 'info', 'warning', 'error', 'debug-low', 'debug-high'
 */
export function addLog(message, type = 'info') {
    // Check if this log level is filtered
    const filterId = getFilterId(type);
    const filterCheckbox = document.getElementById(filterId);
    
    // Debug: uncomment to see filter matching
    // console.log(`Log type: ${type}, Filter ID: ${filterId}, Checkbox:`, filterCheckbox);
    
    if (filterCheckbox && !filterCheckbox.checked) {
        return; // Skip this log entry
    }
    
    const log = document.getElementById('log');
    
    if (!log) {
        console.warn('Log element not found, message:', message);
        return;
    }
    
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    
    const timestamp = new Date().toLocaleTimeString();
    entry.textContent = `[${timestamp}] ${message}`;
    
    log.appendChild(entry);
    log.scrollTop = log.scrollHeight;

    // Keep only last 200 entries
    while (log.children.length > 200) {
        log.removeChild(log.firstChild);
    }
}

/**
 * Clear all log entries
 */
export function clearLog() {
    const log = document.getElementById('log');
    if (log) {
        log.innerHTML = '';
    }
    addLog('Log cleared', 'info');
}