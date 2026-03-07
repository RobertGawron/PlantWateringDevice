let graphData = {
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
    graphCanvas.width = wrapper.clientWidth;
    graphCanvas.height = wrapper.clientHeight || 200;
    drawGraph();
}

/**
 * Add data point to graph
 * @param {number} pump - Pump state
 * @param {number} soil - Soil sensor state
 * @param {number} display - Display clock state
 * @param {number} timestamp - Time in seconds
 */
export function addGraphData(pump, soil, display, timestamp) {
    graphData.pump.push({ time: timestamp, value: pump });
    graphData.soil.push({ time: timestamp, value: soil });
    graphData.display.push({ time: timestamp, value: display });
    
    // Get time window from select
    const timeWindow = parseInt(document.getElementById('graphTimeWindow')?.value || 60);
    const cutoffTime = timestamp - timeWindow;
    
    // Remove old data
    graphData.pump = graphData.pump.filter(d => d.time >= cutoffTime);
    graphData.soil = graphData.soil.filter(d => d.time >= cutoffTime);
    graphData.display = graphData.display.filter(d => d.time >= cutoffTime);
    
    drawGraph();
}

/**
 * Draw the logic analyzer graph
 */
function drawGraph() {
    if (!graphCtx || !graphCanvas) return;
    
    const width = graphCanvas.width;
    const height = graphCanvas.height;
    const signalHeight = height / 3 - 10;
    
    // Clear canvas
    graphCtx.fillStyle = '#1a1a2e';
    graphCtx.fillRect(0, 0, width, height);
    
    // Draw signals
    drawSignal(graphData.pump, '#e74c3c', 0, signalHeight);
    drawSignal(graphData.soil, '#2ecc71', signalHeight + 10, signalHeight);
    drawSignal(graphData.display, '#f39c12', (signalHeight + 10) * 2, signalHeight);
}

/**
 * Draw a single signal trace
 */
function drawSignal(data, color, yOffset, height) {
    if (!graphCtx || data.length === 0) return;
    
    const width = graphCanvas.width;
    const timeWindow = parseInt(document.getElementById('graphTimeWindow')?.value || 60);
    
    graphCtx.strokeStyle = color;
    graphCtx.lineWidth = 2;
    graphCtx.beginPath();
    
    const latestTime = data[data.length - 1]?.time || 0;
    const startTime = latestTime - timeWindow;
    
    data.forEach((point, index) => {
        const x = ((point.time - startTime) / timeWindow) * width;
        const y = yOffset + height - (point.value * (height - 20)) - 10;
        
        if (index === 0) {
            graphCtx.moveTo(x, y);
        } else {
            // Step function (digital signal)
            const prevY = yOffset + height - (data[index - 1].value * (height - 20)) - 10;
            graphCtx.lineTo(x, prevY);
            graphCtx.lineTo(x, y);
        }
    });
    
    graphCtx.stroke();
}

/**
 * Clear all graph data
 */
export function clearGraph() {
    graphData = { pump: [], soil: [], display: [] };
    drawGraph();
}