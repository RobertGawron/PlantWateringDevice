import { gpioState, GPIOPin, type GPIOPinLevel } from './state';

// ------------------------------------------------------------
// Type Definitions
// ------------------------------------------------------------

/**
 * Signal names for the graph
 */
type SignalName = 'button' | 'pump' | 'soil' | 'display';

/**
 * Data point for a signal
 */
interface DataPoint {
    time: number;
    value: GPIOPinLevel;
}

/**
 * Graph data storage
 */
type GraphDataMap = {
    [key in SignalName]: DataPoint[];
};

/**
 * Canvas mapping by signal name
 */
type CanvasMap = {
    [key in SignalName]?: HTMLCanvasElement;
};

/**
 * Rendering context mapping by signal name
 */
type ContextMap = {
    [key in SignalName]?: CanvasRenderingContext2D;
};

/**
 * GPIO state snapshot for graph data
 */
interface GPIOStateSnapshot {
    [GPIOPin.DISPLAY]: GPIOPinLevel;
    [GPIOPin.SOIL_SENSOR]: GPIOPinLevel;
    [GPIOPin.PUMP]: GPIOPinLevel;
    [GPIOPin.BUTTON]: GPIOPinLevel;
}

/**
 * Visible time range
 */
interface VisibleRange {
    startTime: number;
    endTime: number;
    timeWindow: number;
}

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

const SIGNALS: readonly SignalName[] = ['button', 'pump', 'soil', 'display'] as const;

const CANVAS_IDS: Readonly<Record<SignalName, string>> = {
    button: 'buttonGraph',
    pump: 'pumpGraph',
    soil: 'soilGraph',
    display: 'clockGraph'
} as const;

const COLORS_SOLID: Readonly<Record<SignalName, string>> = {
    button: 'rgb(155, 89, 182)',
    pump: 'rgb(231, 76, 60)',
    soil: 'rgb(52, 152, 219)',
    display: 'rgb(243, 156, 18)'
} as const;

// ------------------------------------------------------------
// State
// ------------------------------------------------------------

let canvases: CanvasMap = {};
let contexts: ContextMap = {};
let timeAxisCanvas: HTMLCanvasElement | null = null;
let timeAxisCtx: CanvasRenderingContext2D | null = null;

let graphData: GraphDataMap = {
    button: [],
    pump: [],
    soil: [],
    display: []
};

// Scroll state
let scrollOffset = 0;
let autoScroll = true;
let maxTime = 0;
let isDragging = false;
let dragStartX = 0;
let dragStartOffset = 0;

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------

/**
 * Initialize graph canvases and event listeners
 */
export function initGraph(): void {
    SIGNALS.forEach((sig) => {
        const canvas = document.getElementById(CANVAS_IDS[sig]) as HTMLCanvasElement | null;
        if (canvas) {
            canvases[sig] = canvas;
            const ctx = canvas.getContext('2d');
            if (ctx) {
                contexts[sig] = ctx;
            }

            // Mouse events for scrolling
            canvas.addEventListener('wheel', handleScroll, { passive: false });
            canvas.addEventListener('mousedown', handleDragStart);
            canvas.addEventListener('mousemove', handleDragMove);
            canvas.addEventListener('mouseup', handleDragEnd);
            canvas.addEventListener('mouseleave', handleDragEnd);
        }
    });

    // Time axis canvas
    timeAxisCanvas = document.getElementById('timeAxisCanvas') as HTMLCanvasElement | null;
    if (timeAxisCanvas) {
        timeAxisCtx = timeAxisCanvas.getContext('2d');
    }

    // Control buttons
    document.getElementById('btnScrollStart')?.addEventListener('click', scrollToStart);
    document.getElementById('btnScrollLeft')?.addEventListener('click', () => scrollBy(getTimeWindow() * 0.25));
    document.getElementById('btnScrollRight')?.addEventListener('click', () => scrollBy(-getTimeWindow() * 0.25));
    document.getElementById('btnScrollEnd')?.addEventListener('click', scrollToEnd);
    document.getElementById('btnClearGraph')?.addEventListener('click', clearGraph);

    const autoScrollCheckbox = document.getElementById('chkAutoScroll') as HTMLInputElement | null;
    autoScrollCheckbox?.addEventListener('change', (e: Event) => {
        const target = e.target as HTMLInputElement;
        autoScroll = target.checked;
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

// ------------------------------------------------------------
// Scroll Handling
// ------------------------------------------------------------

/**
 * Handle mouse wheel scrolling
 */
function handleScroll(event: WheelEvent): void {
    event.preventDefault();

    const timeWindow = getTimeWindow();
    const scrollAmount = timeWindow * 0.1;

    if (event.deltaY > 0 || event.deltaX < 0) {
        scrollBy(scrollAmount);
    } else {
        scrollBy(-scrollAmount);
    }
}

/**
 * Handle drag start for scroll
 */
function handleDragStart(event: MouseEvent): void {
    isDragging = true;
    dragStartX = event.clientX;
    dragStartOffset = scrollOffset;
    (event.target as HTMLElement).style.cursor = 'grabbing';
}

/**
 * Handle drag move for scroll
 */
function handleDragMove(event: MouseEvent): void {
    if (!isDragging) return;

    const canvas = event.target as HTMLCanvasElement;
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

/**
 * Handle drag end
 */
function handleDragEnd(event: MouseEvent): void {
    if (isDragging) {
        isDragging = false;
        (event.target as HTMLElement).style.cursor = 'grab';
    }
}

/**
 * Handle keyboard navigation
 */
function handleKeyboard(event: KeyboardEvent): void {
    const target = event.target as HTMLElement;
    if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.tagName === 'SELECT') {
        return;
    }

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

/**
 * Scroll by amount (positive = back in time)
 */
export function scrollBy(amount: number): void {
    autoScroll = false;
    updateAutoScrollCheckbox();

    scrollOffset += amount;
    clampScrollOffset();
    drawGraph();
}

/**
 * Scroll to the start of data
 */
export function scrollToStart(): void {
    autoScroll = false;
    updateAutoScrollCheckbox();

    const timeWindow = getTimeWindow();
    scrollOffset = Math.max(0, maxTime - timeWindow);
    drawGraph();
}

/**
 * Scroll to the end (latest data)
 */
export function scrollToEnd(): void {
    autoScroll = true;
    updateAutoScrollCheckbox();

    scrollOffset = 0;
    drawGraph();
}

/**
 * Update the auto-scroll checkbox state
 */
function updateAutoScrollCheckbox(): void {
    const checkbox = document.getElementById('chkAutoScroll') as HTMLInputElement | null;
    if (checkbox) {
        checkbox.checked = autoScroll;
    }
}

/**
 * Clamp scroll offset to valid range
 */
function clampScrollOffset(): void {
    const timeWindow = getTimeWindow();
    const maxOffset = Math.max(0, maxTime - timeWindow);
    scrollOffset = Math.max(0, Math.min(scrollOffset, maxOffset));
}

/**
 * Get current time window from dropdown
 */
function getTimeWindow(): number {
    const element = document.getElementById('graphTimeWindow') as HTMLSelectElement | null;
    return parseFloat(element?.value || '60');
}

// ------------------------------------------------------------
// Time Formatting
// ------------------------------------------------------------

/**
 * Format seconds into human-readable time string
 */
function formatTime(seconds: number): string {
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

/**
 * Update the scroll position display
 */
function updateScrollPositionDisplay(startTime: number, endTime: number): void {
    const display = document.getElementById('scrollPosition');
    if (display) {
        if (maxTime === 0) {
            display.textContent = 'No data';
        } else {
            display.textContent = `${formatTime(startTime)} — ${formatTime(endTime)} | Total: ${formatTime(maxTime)}`;
        }
    }
}

// ------------------------------------------------------------
// Canvas Management
// ------------------------------------------------------------

/**
 * Resize all canvases to match their container
 */
function resizeGraph(): void {
    SIGNALS.forEach((sig) => {
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

// ------------------------------------------------------------
// Data Management
// ------------------------------------------------------------

/**
 * Add graph data from GPIO state snapshot
 */
export function addGraphDataFromGPIO(gpioSnapshot: GPIOStateSnapshot, timestamp: number): void {
    graphData.display.push({ time: timestamp, value: gpioSnapshot[GPIOPin.DISPLAY] });
    graphData.soil.push({ time: timestamp, value: gpioSnapshot[GPIOPin.SOIL_SENSOR] });
    graphData.pump.push({ time: timestamp, value: gpioSnapshot[GPIOPin.PUMP] });
    graphData.button.push({ time: timestamp, value: gpioSnapshot[GPIOPin.BUTTON] });

    maxTime = Math.max(maxTime, timestamp);

    if (autoScroll) {
        scrollOffset = 0;
    }

    drawGraph();
}

/**
 * Calculate visible time range
 */
function getVisibleRange(): VisibleRange {
    const timeWindow = getTimeWindow();
    let endTime: number;
    let startTime: number;

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

// ------------------------------------------------------------
// Drawing
// ------------------------------------------------------------

/**
 * Draw all graphs
 */
function drawGraph(): void {
    const { startTime, endTime, timeWindow } = getVisibleRange();

    updateScrollPositionDisplay(startTime, endTime);

    SIGNALS.forEach((sig) => {
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
        const visibleData = data.filter((p) => p.time >= startTime && p.time <= endTime);

        // Find the last point BEFORE the visible window (for initial state)
        let pointBeforeWindow: DataPoint | null = null;
        for (let i = data.length - 1; i >= 0; i--) {
            if (data[i].time < startTime) {
                pointBeforeWindow = data[i];
                break;
            }
        }

        // Find the first point AFTER the visible window (to know if we should extend)
        let pointAfterWindow: DataPoint | null = null;
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

        let lastY: number | null = null;
        let lastX: number | null = null;
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
                ctx.lineTo(x, lastY!);
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

/**
 * Draw grid for a signal
 */
function drawSignalGrid(
    ctx: CanvasRenderingContext2D,
    canvas: HTMLCanvasElement,
    startTime: number,
    timeWindow: number
): void {
    ctx.strokeStyle = 'rgba(0, 0, 0, 0.06)';
    ctx.lineWidth = 1;

    // Determine grid interval
    let gridInterval: number;
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

/**
 * Draw shared time axis
 */
function drawTimeAxis(startTime: number, timeWindow: number): void {
    if (!timeAxisCtx || !timeAxisCanvas) return;

    const ctx = timeAxisCtx;
    const canvas = timeAxisCanvas;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = '#333';
    ctx.font = '10px system-ui, -apple-system, sans-serif';
    ctx.textAlign = 'center';

    // Determine label interval
    let labelInterval: number;
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
        let label: string;
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

// ------------------------------------------------------------
// Clear
// ------------------------------------------------------------

/**
 * Clear all graph data
 */
export function clearGraph(): void {
    SIGNALS.forEach((sig) => {
        graphData[sig] = [];
    });

    scrollOffset = 0;
    autoScroll = true;
    maxTime = 0;
    updateAutoScrollCheckbox();
    drawGraph();
}

// ------------------------------------------------------------
// Exports
// ------------------------------------------------------------

