// js/graph.js - Logic analyzer graph

// Signal colors (semi-transparent for overlap visibility)
const SIGNAL_COLORS = {
    button: 'rgba(155, 89, 182, 0.7)',   // Purple
    pump: 'rgba(231, 76, 60, 0.7)',      // Red
    soil: 'rgba(52, 152, 219, 0.7)',     // Blue
    display: 'rgba(243, 156, 18, 0.7)'   // Orange/Yellow
};

// Solid colors for lines
const SIGNAL_COLORS_SOLID = {
    button: 'rgb(155, 89, 182)',
    pump: 'rgb(231, 76, 60)',
    soil: 'rgb(52, 152, 219)',
    display: 'rgb(243, 156, 18)'
};

// Graph configuration
const GRAPH_CONFIG = {
    backgroundColor: '#f5f5f5',
    gridColor: 'rgba(0, 0, 0, 0.08)',
    lineWidth: 1.5,         // Thinner lines
    signalPadding: 4
};

// Graph data storage
let graphData = {
    button: [],
    pump: [],
    soil: [],
    display: []
};

let graphCanvas = null;
let graphCtx = null;

/**
 * Initialize the logic analyzer graph
 */
export function initGraph() {
    graphCanvas = document.getElementById('logicAnalyzer');
    if (graphCanvas) {
        graphCtx = graphCanvas.getContext('2d');
        resizeGraph();
        window.addEventListener('resize', resizeGraph);
    }
}

/**
 * Resize graph canvas to fit container
 */
function resizeGraph() {
    if (!graphCanvas) return;
    
    const wrapper = graphCanvas.parentElement;
    const rect = wrapper.getBoundingClientRect();
    
    graphCanvas.width = rect.width;
    graphCanvas.height = rect.height;
    
    drawGraph();
}

/**
 * Add data point to graph
 */
export function addGraphData(button, pump, soil, display, timestamp) {
    graphData.button.push({ time: timestamp, value: button });
    graphData.pump.push({ time: timestamp, value: pump });
    graphData.soil.push({ time: timestamp, value: soil });
    graphData.display.push({ time: timestamp, value: display });
    
    const timeWindow = parseInt(document.getElementById('graphTimeWindow')?.value || 60);
    const cutoffTime = timestamp - timeWindow;
    
    graphData.button = graphData.button.filter(d => d.time >= cutoffTime);
    graphData.pump = graphData.pump.filter(d => d.time >= cutoffTime);
    graphData.soil = graphData.soil.filter(d => d.time >= cutoffTime);
    graphData.display = graphData.display.filter(d => d.time >= cutoffTime);
    
    drawGraph();
}

/**
 * Draw the complete graph
 */
function drawGraph() {
    if (!graphCtx || !graphCanvas) return;
    
    const width = graphCanvas.width;
    const height = graphCanvas.height;
    
    // Clear canvas
    graphCtx.fillStyle = GRAPH_CONFIG.backgroundColor;
    graphCtx.fillRect(0, 0, width, height);
    
    // Draw grid
    drawGrid(width, height);
    
    // Calculate signal lane height
    const numSignals = 4;
    const totalPadding = GRAPH_CONFIG.signalPadding * (numSignals + 1);
    const laneHeight = (height - totalPadding) / numSignals;
    
    // Draw each signal
    const signals = [
        { data: graphData.button, color: SIGNAL_COLORS.button, solidColor: SIGNAL_COLORS_SOLID.button },
        { data: graphData.pump, color: SIGNAL_COLORS.pump, solidColor: SIGNAL_COLORS_SOLID.pump },
        { data: graphData.soil, color: SIGNAL_COLORS.soil, solidColor: SIGNAL_COLORS_SOLID.soil },
        { data: graphData.display, color: SIGNAL_COLORS.display, solidColor: SIGNAL_COLORS_SOLID.display }
    ];
    
    signals.forEach((signal, index) => {
        const yOffset = GRAPH_CONFIG.signalPadding + index * (laneHeight + GRAPH_CONFIG.signalPadding);
        drawSignal(signal.data, signal.color, signal.solidColor, yOffset, laneHeight, width);
    });
}

/**
 * Draw background grid
 */
function drawGrid(width, height) {
    graphCtx.strokeStyle = GRAPH_CONFIG.gridColor;
    graphCtx.lineWidth = 1;
    
    // Vertical lines
    const numVerticalLines = 10;
    const xStep = width / numVerticalLines;
    
    for (let i = 0; i <= numVerticalLines; i++) {
        const x = i * xStep;
        graphCtx.beginPath();
        graphCtx.moveTo(x, 0);
        graphCtx.lineTo(x, height);
        graphCtx.stroke();
    }
    
    // Horizontal lines (signal lane separators)
    const numSignals = 4;
    const laneHeight = height / numSignals;
    
    for (let i = 1; i < numSignals; i++) {
        const y = i * laneHeight;
        graphCtx.beginPath();
        graphCtx.moveTo(0, y);
        graphCtx.lineTo(width, y);
        graphCtx.stroke();
    }
}

/**
 * Draw a single signal trace
 */
function drawSignal(data, fillColor, strokeColor, yOffset, laneHeight, width) {
    if (data.length === 0) return;
    
    const timeWindow = parseInt(document.getElementById('graphTimeWindow')?.value || 60);
    const latestTime = data[data.length - 1]?.time || 0;
    const startTime = latestTime - timeWindow;
    
    // Calculate Y positions for HIGH and LOW
    const padding = 6;
    const highY = yOffset + padding;
    const lowY = yOffset + laneHeight - padding;
    
    // Draw filled area under signal
    graphCtx.fillStyle = fillColor;
    graphCtx.beginPath();
    
    let firstPoint = true;
    let lastX = 0;
    let lastY = lowY;
    
    data.forEach((point) => {
        const x = ((point.time - startTime) / timeWindow) * width;
        const y = point.value ? highY : lowY;
        
        if (firstPoint) {
            graphCtx.moveTo(x, lowY);
            graphCtx.lineTo(x, y);
            firstPoint = false;
        } else {
            graphCtx.lineTo(x, lastY);
            graphCtx.lineTo(x, y);
        }
        
        lastX = x;
        lastY = y;
    });
    
    // Complete the fill area
    graphCtx.lineTo(lastX, lowY);
    graphCtx.closePath();
    graphCtx.fill();
    
    // Draw signal line on top
    graphCtx.strokeStyle = strokeColor;
    graphCtx.lineWidth = GRAPH_CONFIG.lineWidth;
    graphCtx.lineCap = 'square';
    graphCtx.beginPath();
    
    firstPoint = true;
    lastY = lowY;
    
    data.forEach((point) => {
        const x = ((point.time - startTime) / timeWindow) * width;
        const y = point.value ? highY : lowY;
        
        if (firstPoint) {
            graphCtx.moveTo(x, y);
            firstPoint = false;
        } else {
            graphCtx.lineTo(x, lastY);
            graphCtx.lineTo(x, y);
        }
        
        lastY = y;
    });
    
    graphCtx.stroke();
}

/**
 * Clear all graph data
 */
export function clearGraph() {
    graphData = {
        button: [],
        pump: [],
        soil: [],
        display: []
    };
    drawGraph();
}

/**
 * Get current graph data (for debugging)
 */
export function getGraphData() {
    return graphData;
}