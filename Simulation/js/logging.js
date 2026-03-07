/**
 * Add a log entry to the system log
 * @param {string} message - Log message
 * @param {string} type - Log type: 'info', 'warning', 'error', 'debug-low', 'debug-high'
 */
export function addLog(message, type = 'info') {
    // Check if this log level is filtered
    const typeNormalized = type.replace('-', '');
    const filterId = `filter${typeNormalized.charAt(0).toUpperCase() + typeNormalized.slice(1)}`;
    const filterCheckbox = document.getElementById(filterId);
    
    if (filterCheckbox && !filterCheckbox.checked) {
        return; // Skip this log entry
    }
    
    const log = document.getElementById('log');
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
    document.getElementById('log').innerHTML = '';
    addLog('Log cleared', 'info');
}