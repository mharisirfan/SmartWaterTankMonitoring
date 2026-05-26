// Dashboard State
const state = {
    history: [],
    maxHistoryPoints: 60,
    updateCount: 0,
    chart: null,
    pollInterval: null
};

// Initialize dashboard
document.addEventListener('DOMContentLoaded', () => {
    console.log('🚀 Dashboard initialized');
    initChart();
    startPolling();
});

// Poll API for latest status
function startPolling() {
    // Poll immediately
    fetchStatus();
    
    // Then poll every 500ms for updates
    state.pollInterval = setInterval(fetchStatus, 500);
}

async function fetchStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();
        updateDashboard(data);
    } catch (error) {
        console.error('❌ Fetch error:', error);
        updateMQTTStatus(false);
    }
}

// Update all dashboard elements
function updateDashboard(data) {
    // Update MQTT connection status
    updateMQTTStatus(data.connected);
    
    if (data.timestamp) {
        state.updateCount++;
        
        // Add to history
        addToHistory({
            level: data.waterLevel,
            distance: data.distance,
            timestamp: new Date(data.timestamp)
        });
        
        // Update gauge
        updateGauge(data.waterLevel);
        
        // Update status badge
        updateStatusBadge(data.status);
        
        // Update all values
        document.getElementById('water-level').textContent = `${data.waterLevel}%`;
        document.getElementById('distance-value').textContent = `${data.distance.toFixed(1)} cm`;
        document.getElementById('water-height-value').textContent = `${data.waterHeight.toFixed(2)} cm`;
        document.getElementById('motor-value').textContent = data.motor;
        document.getElementById('red-sensor-value').textContent = data.redSensorConfirmed;
        document.getElementById('last-update').textContent = formatTime(new Date(data.timestamp));
        document.getElementById('footer-time').textContent = formatTime(new Date(data.timestamp));
        
        // Update stats
        document.getElementById('stat-level').textContent = data.waterLevel;
        document.getElementById('stat-distance').textContent = data.distance.toFixed(1);
        document.getElementById('stat-motor').textContent = data.motor;
        document.getElementById('stat-updates').textContent = state.updateCount;
        
        // Update raw data
        document.getElementById('raw-data').textContent = JSON.stringify(data, null, 2);
        
        // Update red sensor status styling
        const redSensorRow = document.getElementById('red-sensor-status');
        if (data.redSensorConfirmed === 'WATER_DETECTED') {
            redSensorRow.classList.add('alert');
            redSensorRow.style.background = '#f8d7da';
            redSensorRow.style.borderLeftColor = '#dc3545';
        } else {
            redSensorRow.classList.remove('alert');
            redSensorRow.style.background = 'transparent';
            redSensorRow.style.borderLeftColor = 'transparent';
        }
        
        // Update chart
        updateChart();
    }
}

// Update gauge visualization
function updateGauge(level) {
    const arc = document.getElementById('gauge-arc');
    const circumference = 314; // 2 * π * 50 (approximate for SVG path)
    const progress = (level / 100) * circumference;
    arc.style.strokeDasharray = `${progress}, ${circumference}`;
    
    // Change color based on level
    if (level < 30) {
        arc.style.stroke = '#dc3545'; // Red
    } else if (level < 70) {
        arc.style.stroke = '#ffc107'; // Yellow
    } else {
        arc.style.stroke = '#28a745'; // Green
    }
}

// Update status badge
function updateStatusBadge(status) {
    const badge = document.getElementById('status-badge');
    badge.textContent = status;
    badge.className = `status-badge ${status}`;
}

// Update MQTT connection status
function updateMQTTStatus(connected) {
    const dot = document.getElementById('mqtt-status');
    const text = document.getElementById('mqtt-text');
    
    if (connected) {
        dot.className = 'status-dot connected';
        text.textContent = 'MQTT Connected';
        text.style.color = '#28a745';
    } else {
        dot.className = 'status-dot disconnected';
        text.textContent = 'MQTT Disconnected';
        text.style.color = '#dc3545';
    }
}

// Add data point to history
function addToHistory(point) {
    state.history.push(point);
    
    // Keep only last N points
    if (state.history.length > state.maxHistoryPoints) {
        state.history.shift();
    }
}

// Initialize chart
function initChart() {
    const ctx = document.getElementById('historyChart').getContext('2d');
    
    state.chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Water Level (%)',
                    data: [],
                    borderColor: '#667eea',
                    backgroundColor: 'rgba(102, 126, 234, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4,
                    pointRadius: 3,
                    pointBackgroundColor: '#667eea',
                    pointBorderColor: '#fff',
                    pointBorderWidth: 2,
                    yAxisID: 'y'
                },
                {
                    label: 'Distance (cm)',
                    data: [],
                    borderColor: '#764ba2',
                    backgroundColor: 'rgba(118, 75, 162, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4,
                    pointRadius: 3,
                    pointBackgroundColor: '#764ba2',
                    pointBorderColor: '#fff',
                    pointBorderWidth: 2,
                    yAxisID: 'y1'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: {
                mode: 'index',
                intersect: false
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: {
                        usePointStyle: true,
                        padding: 15,
                        font: {
                            size: 12,
                            weight: '600'
                        }
                    }
                },
                filler: {
                    propagate: true
                }
            },
            scales: {
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    title: {
                        display: true,
                        text: 'Water Level (%)',
                        font: { weight: 'bold' }
                    },
                    min: 0,
                    max: 100
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    title: {
                        display: true,
                        text: 'Distance (cm)',
                        font: { weight: 'bold' }
                    },
                    grid: {
                        drawOnChartArea: false
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: 'Time',
                        font: { weight: 'bold' }
                    }
                }
            }
        }
    });
}

// Update chart with new data
function updateChart() {
    if (!state.chart) return;
    
    // Update labels (times)
    state.chart.data.labels = state.history.map(point => 
        formatTime(point.timestamp)
    );
    
    // Update data
    state.chart.data.datasets[0].data = state.history.map(point => point.level);
    state.chart.data.datasets[1].data = state.history.map(point => point.distance);
    
    state.chart.update('none'); // Update without animation for smooth real-time
}

// Format time for display
function formatTime(date) {
    return date.toLocaleTimeString('en-US', {
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        hour12: false
    });
}

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (state.pollInterval) {
        clearInterval(state.pollInterval);
    }
});

console.log('✅ Dashboard app loaded');
