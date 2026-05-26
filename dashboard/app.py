from flask import Flask, render_template, jsonify
from flask_cors import CORS
import paho.mqtt.client as mqtt
import json
import threading
import time
from datetime import datetime

app = Flask(__name__)
CORS(app)

# MQTT Configuration
MQTT_BROKER = "192.168.15.2"
MQTT_PORT = 1883
MQTT_TOPIC = "water_tank/status"

# Storage for latest data
latest_data = {
    "waterLevel": 0,
    "distance": 0,
    "waterHeight": 0,
    "status": "UNKNOWN",
    "redSensorConfirmed": "NO_WATER",
    "motor": "OFF",
    "timestamp": None,
    "connected": False
}

# Lock for thread-safe access
data_lock = threading.Lock()

def on_connect(client, userdata, flags, rc):
    """MQTT connection callback"""
    if rc == 0:
        print(f"✓ Connected to MQTT broker at {MQTT_BROKER}")
        with data_lock:
            latest_data["connected"] = True
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"✗ Connection failed with code {rc}")
        with data_lock:
            latest_data["connected"] = False

def on_disconnect(client, userdata, rc):
    """MQTT disconnection callback"""
    with data_lock:
        latest_data["connected"] = False
    if rc != 0:
        print(f"✗ Unexpected disconnection with code {rc}")

def on_message(client, userdata, msg):
    """MQTT message callback"""
    try:
        payload = json.loads(msg.payload.decode())
        with data_lock:
            latest_data.update({
                "waterLevel": payload.get("level", 0),
                "distance": payload.get("distance_cm", 0),
                "waterHeight": payload.get("water_height_cm", 0),
                "status": payload.get("status", "UNKNOWN"),
                "redSensorConfirmed": payload.get("red_sensor_confirmed", "NO_WATER"),
                "motor": payload.get("motor", "OFF"),
                "timestamp": datetime.now().isoformat()
            })
        print(f"✓ Updated: Level={payload.get('level')}%, Status={payload.get('status')}, Motor={payload.get('motor')}")
    except json.JSONDecodeError:
        print(f"✗ Failed to decode message: {msg.payload}")
    except Exception as e:
        print(f"✗ Error processing message: {e}")

def mqtt_thread():
    """MQTT client in separate thread"""
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    
    try:
        print(f"Connecting to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
        client.loop_forever()
    except Exception as e:
        print(f"✗ MQTT connection error: {e}")

@app.route('/')
def index():
    """Serve dashboard"""
    return render_template('index.html')

@app.route('/api/status')
def get_status():
    """API endpoint for current status"""
    with data_lock:
        return jsonify(latest_data)

if __name__ == '__main__':
    # Start MQTT thread
    mqtt_client = threading.Thread(target=mqtt_thread, daemon=True)
    mqtt_client.start()
    
    # Give MQTT thread time to connect
    time.sleep(1)
    
    # Start Flask app
    print("\n🚀 Dashboard running at http://localhost:5000")
    print("📊 Listening for MQTT messages on:", MQTT_TOPIC)
    app.run(debug=False, host='0.0.0.0', port=5000)
