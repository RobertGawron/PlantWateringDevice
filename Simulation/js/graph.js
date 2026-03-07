const SIGNALS = ['button', 'pump', 'soil', 'display'];
const CANVAS_IDS = {
    button: 'buttonGraph',
    pump: 'pumpGraph',
    soil: 'soilGraph',
    display: 'clockGraph'
};

let canvases = {};
let contexts = {};
let timeAxisCanvas = null;
let timeAxisCtx = null;

let graphData = {
    button: [],
    pump: [],
    soil: [],
    display: []
};

const COLORS_SOLID = {
    button: 'rgb(155, 89, 182)',
    pump: 'rgb(231, 76, 60)',
    soil: 'rgb(52, 152, 219)',
    display: 'rgb(243, 156, 18)'
};

// Scroll state
let scrollOffset = 0;
let autoScroll = true;
let maxTime = 0;
let isDragging = false;
let dragStartX = 0;
let dragStartOffset = 0;

// Initialize graph
export function initGraph() {
    SIGNALS.forEach(sig => {
        const canvas = document.getElementById(CANVAS_IDS[sig]);
        if (canvas) {
            canvases[sig] = canvas;
            contexts[sig] = canvas.getContext('2d');

            // Mouse events for scrolling
            canvas.addEventListener('wheel', handleScroll, { passive: false });
            canvas.addEventListener('mousedown', handleDragStart);
            canvas.addEventListener('mousemove', handleDragMove);
            canvas.addEventListener('mouseup', handleDragEnd);
            canvas.addEventListener('mouseleave', handleDragEnd);
        }
    });

    // Time axis canvas
    timeAxisCanvas = document.getElementById('timeAxisCanvas');
    if (timeAxisCanvas) {
        timeAxisCtx = timeAxisCanvas.getContext('2d');
    }

    // Control buttons
    document.getElementById('btnScrollStart')?.addEventListener('click', scrollToStart);
    document.getElementById('btnScrollLeft')?.addEventListener('click', () => scrollBy(getTimeWindow() * 0.25));
    document.getElementById('btnScrollRight')?.addEventListener('click', () => scrollBy(-getTimeWindow() * 0.25));
    document.getElementById('btnScrollEnd')?.addEventListener('click', scrollToEnd);
    document.getElementById('btnClearGraph')?.addEventListener('click', clearGraph);

    document.getElementById('chkAutoScroll')?.addEventListener('change', (e) => {
        autoScroll = e.target.checked;
        if (autoScroll) {
            scrollOffset = 0;
            drawGraph();
        }
    });

    // Time window change
    document.getElementById('graphTimeWindow')?.addEventListener('change', () => {
        clampScrollOffset();
        drawGraph();
    });

    // Keyboard navigation
    document.addEventListener('keydown', handleKeyboard);

    window.addEventListener('resize', resizeGraph);
    resizeGraph();
}

// Handle mouse wheel scrolling
function handleScroll(event) {
    event.preventDefault();

    const timeWindow = getTimeWindow();
    const scrollAmount = timeWindow * 0.1;

    if (event.deltaY > 0 || event.deltaX < 0) {
        scrollBy(scrollAmount);
    } else {
        scrollBy(-scrollAmount);
    }
}

// Drag to scroll
function handleDragStart(event) {
    isDragging = true;
    dragStartX = event.clientX;
    dragStartOffset = scrollOffset;
    event.target.style.cursor = 'grabbing';
}

function handleDragMove(event) {
    if (!isDragging) return;

    const canvas = event.target;
    const dx = event.clientX - dragStartX;
    const timeWindow = getTimeWindow();
    const pixelsPerSecond = canvas.width / timeWindow;
    const timeDelta = dx / pixelsPerSecond;

    autoScroll = false;
    updateAutoScrollCheckbox();

    scrollOffset = dragStartOffset + timeDelta;
    clampScrollOffset();
    drawGraph();
}

function handleDragEnd(event) {
    if (isDragging) {
        isDragging = false;
        event.target.style.cursor = 'grab';
    }
}

// Handle keyboard navigation
function handleKeyboard(event) {
    if (event.target.tagName === 'INPUT' || event.target.tagName === 'TEXTAREA' || event.target.tagName === 'SELECT') return;

    const timeWindow = getTimeWindow();

    switch (event.key) {
        case 'ArrowLeft':
            scrollBy(event.shiftKey ? timeWindow * 0.5 : timeWindow * 0.1);
            event.preventDefault();
            break;
        case 'ArrowRight':
            scrollBy(event.shiftKey ? -timeWindow * 0.5 : -timeWindow * 0.1);
            event.preventDefault();
            break;
        case 'Home':
            scrollToStart();
            event.preventDefault();
            break;
        case 'End':
            scrollToEnd();
            event.preventDefault();
            break;
    }
}

// Scroll by amount (positive = back in time)
function scrollBy(amount) {
    autoScroll = false;
    updateAutoScrollCheckbox();

    scrollOffset += amount;
    clampScrollOffset();
    drawGraph();
}

function scrollToStart() {
    autoScroll = false;
    updateAutoScrollCheckbox();

    const timeWindow = getTimeWindow();
    scrollOffset = Math.max(0, maxTime - timeWindow);
    drawGraph();
}

function scrollToEnd() {
    autoScroll = true;
    updateAutoScrollCheckbox();

    scrollOffset = 0;
    drawGraph();
}

function updateAutoScrollCheckbox() {
    const checkbox = document.getElementById('chkAutoScroll');
    if (checkbox) checkbox.checked = autoScroll;
}

function clampScrollOffset() {
    const timeWindow = getTimeWindow();
    const maxOffset = Math.max(0, maxTime - timeWindow);
    scrollOffset = Math.max(0, Math.min(scrollOffset, maxOffset));
}

function getTimeWindow() {
    return parseFloat(document.getElementById('graphTimeWindow')?.value || 60);
}

// Format time display
function formatTime(seconds) {
    if (seconds >= 3600) {
        const h = Math.floor(seconds / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        const s = Math.floor(seconds % 60);
        return `${h}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
    } else if (seconds >= 60) {
        const m = Math.floor(seconds / 60);
        const s = (seconds % 60).toFixed(1);
        return `${m}:${parseFloat(s).toFixed(1).padStart(4, '0')}`;
    }
    return `${seconds.toFixed(2)}s`;
}

function updateScrollPositionDisplay(startTime, endTime) {
    const display = document.getElementById('scrollPosition');
    if (display) {
        if (maxTime === 0) {
            display.textContent = 'No data';
        } else {
            display.textContent = `${formatTime(startTime)} — ${formatTime(endTime)} | Total: ${formatTime(maxTime)}`;
        }
    }
}

// Resize canvases
function resizeGraph() {
    SIGNALS.forEach(sig => {
        const canvas = canvases[sig];
        if (!canvas) return;

        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
    });

    if (timeAxisCanvas) {
        const rect = timeAxisCanvas.getBoundingClientRect();
        timeAxisCanvas.width = rect.width;
        timeAxisCanvas.height = rect.height;
    }

    drawGraph();
}

// Add data from GPIO
export function addGraphDataFromGPIO(gpioState, timestamp) {
    graphData.display.push({ time: timestamp, value: gpioState[0] });
    graphData.soil.push({ time: timestamp, value: gpioState[1] });
    graphData.pump.push({ time: timestamp, value: gpioState[2] });
    graphData.button.push({ time: timestamp, value: gpioState[3] });

    maxTime = Math.max(maxTime, timestamp);

    if (autoScroll) {
        scrollOffset = 0;
    }

    drawGraph();
}

// Calculate visible time range
function getVisibleRange() {
    const timeWindow = getTimeWindow();
    let endTime, startTime;

    if (maxTime === 0) {
        // No data yet
        return { startTime: 0, endTime: timeWindow, timeWindow };
    }

    if (autoScroll) {
        endTime = Math.max(timeWindow, maxTime);
        startTime = endTime - timeWindow;
    } else {
        endTime = Math.max(timeWindow, maxTime - scrollOffset);
        startTime = endTime - timeWindow;

        if (startTime < 0) {
            startTime = 0;
            endTime = timeWindow;
        }
    }

    return { startTime, endTime, timeWindow };
}

// Draw all graphs
function drawGraph() {
    const { startTime, endTime, timeWindow } = getVisibleRange();

    updateScrollPositionDisplay(startTime, endTime);

    SIGNALS.forEach(sig => {
        const ctx = contexts[sig];
        const canvas = canvases[sig];
        if (!ctx || !canvas) return;

        ctx.clearRect(0, 0, canvas.width, canvas.height);

        // Always draw grid (even without data)
        drawSignalGrid(ctx, canvas, startTime, timeWindow);

        const data = graphData[sig];

        // Don't draw anything if no data exists
        if (data.length === 0) return;

        // Get actual data time bounds
        const dataStartTime = data[0].time;
        const dataEndTime = data[data.length - 1].time;

        // If visible window is entirely before any data exists, don't draw
        if (endTime < dataStartTime) return;

        // If visible window is entirely after all data, draw last known state
        if (startTime > dataEndTime) {
            const lastPoint = data[data.length - 1];
            const highY = 8;
            const lowY = canvas.height - 8;
            const y = lastPoint.value ? highY : lowY;

            ctx.strokeStyle = COLORS_SOLID[sig];
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
            ctx.stroke();
            return;
        }

        // Filter to visible data points
        const visibleData = data.filter(p => p.time >= startTime && p.time <= endTime);

        // Find the last point BEFORE the visible window (for initial state)
        let pointBeforeWindow = null;
        for (let i = data.length - 1; i >= 0; i--) {
            if (data[i].time < startTime) {
                pointBeforeWindow = data[i];
                break;
            }
        }

        // Find the first point AFTER the visible window (to know if we should extend)
        let pointAfterWindow = null;
        for (let i = 0; i < data.length; i++) {
            if (data[i].time > endTime) {
                pointAfterWindow = data[i];
                break;
            }
        }

        ctx.strokeStyle = COLORS_SOLID[sig];
        ctx.lineWidth = 2;
        ctx.beginPath();

        const highY = 8;
        const lowY = canvas.height - 8;

        let lastY = null;
        let lastX = null;
        let started = false;

        // If there's data before this window, start from the left edge with that state
        if (pointBeforeWindow !== null) {
            lastY = pointBeforeWindow.value ? highY : lowY;
            ctx.moveTo(0, lastY);
            started = true;
        }

        // Draw each visible data point
        visibleData.forEach((point) => {
            const x = ((point.time - startTime) / timeWindow) * canvas.width;
            const y = point.value ? highY : lowY;

            if (!started) {
                // First point ever - start here, don't draw from edge
                ctx.moveTo(x, y);
                started = true;
            } else {
                // Step function: horizontal to this x, then vertical to new y
                ctx.lineTo(x, lastY);
                ctx.lineTo(x, y);
            }

            lastX = x;
            lastY = y;
        });

        // Extend to right edge ONLY if there's data after this window
        // (meaning we're viewing historical data, not the live edge)
        if (started && lastY !== null && pointAfterWindow !== null) {
            ctx.lineTo(canvas.width, lastY);
        }

        ctx.stroke();
    });

    // Draw time axis
    drawTimeAxis(startTime, timeWindow);
}

// Draw grid for signal
function drawSignalGrid(ctx, canvas, startTime, timeWindow) {
    ctx.strokeStyle = 'rgba(0, 0, 0, 0.06)';
    ctx.lineWidth = 1;

    // Determine grid interval
    let gridInterval;
    if (timeWindow <= 10) gridInterval = 1;
    else if (timeWindow <= 30) gridInterval = 5;
    else if (timeWindow <= 60) gridInterval = 10;
    else if (timeWindow <= 300) gridInterval = 30;
    else if (timeWindow <= 600) gridInterval = 60;
    else gridInterval = 300;

    const firstGrid = Math.ceil(startTime / gridInterval) * gridInterval;

    for (let t = firstGrid; t <= startTime + timeWindow; t += gridInterval) {
        const x = ((t - startTime) / timeWindow) * canvas.width;
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, canvas.height);
        ctx.stroke();
    }
}

// Draw shared time axis
function drawTimeAxis(startTime, timeWindow) {
    if (!timeAxisCtx || !timeAxisCanvas) return;

    const ctx = timeAxisCtx;
    const canvas = timeAxisCanvas;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = '#333';
    ctx.font = '10px system-ui, -apple-system, sans-serif';
    ctx.textAlign = 'center';

    // Determine label interval
    let labelInterval;
    if (timeWindow <= 10) labelInterval = 1;
    else if (timeWindow <= 30) labelInterval = 5;
    else if (timeWindow <= 60) labelInterval = 10;
    else if (timeWindow <= 300) labelInterval = 30;
    else if (timeWindow <= 600) labelInterval = 60;
    else labelInterval = 300;

    const firstLabel = Math.ceil(startTime / labelInterval) * labelInterval;

    for (let t = firstLabel; t <= startTime + timeWindow; t += labelInterval) {
        const x = ((t - startTime) / timeWindow) * canvas.width;

        // Tick mark
        ctx.strokeStyle = '#999';
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, 4);
        ctx.stroke();

        // Label
        let label;
        if (t >= 3600) {
            const h = Math.floor(t / 3600);
            const m = Math.floor((t % 3600) / 60);
            label = `${h}h${m > 0 ? m + 'm' : ''}`;
        } else if (t >= 60) {
            const m = Math.floor(t / 60);
            const s = t % 60;
            label = s === 0 ? `${m}m` : `${m}:${s.toString().padStart(2, '0')}`;
        } else {
            label = `${t}s`;
        }

        ctx.fillText(label, x, 16);
    }
}

// Clear all graphs
export function clearGraph() {
    SIGNALS.forEach(sig => {
        graphData[sig] = [];
    });

    scrollOffset = 0;
    autoScroll = true;
    maxTime = 0;
    updateAutoScrollCheckbox();
    drawGraph();
}

export { scrollToStart, scrollToEnd, scrollBy };