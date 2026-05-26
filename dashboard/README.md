# 💧 Water Tank Dashboard

Real-time web dashboard for monitoring your water tank system via MQTT.

## Features

✨ **Live Updates** - Real-time water level monitoring with 2-second refresh rate
📊 **Interactive Charts** - 60-second historical data visualization
🎯 **Gauge Visualization** - Dynamic water level gauge with color-coded status
📱 **Responsive Design** - Works on desktop, tablet, and mobile devices
🔌 **MQTT Integration** - Automatic connection and message handling
⚡ **Zero Configuration** - Auto-detects your MQTT broker settings

## Quick Start

### Prerequisites
- Python 3.7+
- Your MQTT broker running at `192.168.15.2:1883`
- Water tank system publishing to `water_tank/status` topic

### Installation

1. Install dependencies:
```bash
pip install -r requirements.txt
```

2. Run the dashboard:
```bash
python app.py
```

3. Open your browser and navigate to:
```
http://localhost:5000
```

## Dashboard Displays

### Water Level Gauge
- Visual representation of tank fill percentage
- Color-coded: Red (low) → Yellow (medium) → Green (full)

### System Status
Shows real-time data:
- **Distance**: Ultrasonic sensor reading (cm)
- **Water Height**: Calculated water column height (cm)
- **Motor**: Pump motor status (ON/OFF)
- **Red Sensor**: Water detection confirmation
- **Status**: Current system state

### Quick Stats
- Water level percentage
- Distance measurement
- Motor state
- Update counter

### History Chart
- Last 60 seconds of water level data
- Distance trend overlay
- Time-stamped data points

### Raw Data
Live JSON display of incoming MQTT messages

## MQTT Message Format

The dashboard expects messages in this format:

```json
{
  "distance_cm": 5.42,
  "water_height_cm": 9.71,
  "level": 84,
  "status": "NORMAL",
  "red_sensor_confirmed": "NO_WATER",
  "motor": "ON"
}
```

## Configuration

### Change MQTT Broker
Edit `app.py` and update:
```python
MQTT_BROKER = "YOUR_BROKER_IP"
MQTT_PORT = 1883
MQTT_TOPIC = "water_tank/status"
```

### Change Dashboard Port
Modify the last line in `app.py`:
```python
app.run(debug=False, host='0.0.0.0', port=8000)  # Change 5000 to your port
```

## Troubleshooting

### Dashboard shows "MQTT Disconnected"
- Check MQTT broker IP address
- Ensure MQTT broker is running
- Verify network connectivity

### No data appearing
- Check that water tank system is publishing messages
- Verify topic name matches `water_tank/status`
- Check browser console for errors (F12 → Console)

### Connection refused
- MQTT broker not running
- Wrong IP address in `app.py`
- Firewall blocking port 1883

## File Structure

```
dashboard/
├── app.py                 # Flask backend & MQTT handler
├── requirements.txt       # Python dependencies
├── templates/
│   └── index.html        # Dashboard HTML
└── static/
    ├── css/
    │   └── style.css     # Dashboard styling
    └── js/
        └── app.js        # Frontend logic & real-time updates
```

## Architecture

```
Water Tank System (ESP8266)
        ↓ MQTT publish
    MQTT Broker
        ↓ MQTT subscribe
    Flask Backend (app.py)
        ↓ WebSocket/REST API
    Web Dashboard
        ↓ Real-time display
    Browser (http://localhost:5000)
```

## Performance Notes

- Updates every 500ms for smooth real-time visualization
- Maintains 60-second historical data window
- Lightweight (~5MB memory footprint)
- Auto-reconnects on MQTT disconnect

## API Endpoints

- `GET /` - Main dashboard page
- `GET /api/status` - Current system status (JSON)

## License

MIT

---

**Need help?** Check the browser console (F12) for error messages and MQTT connection logs appear in the terminal.
