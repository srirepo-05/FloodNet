#  FloodCast: Real-Time Flood Prediction System

FloodCast is an IoT-enabled, machine learning-powered flood prediction and visualization platform. It integrates a microcontroller-based sensor system with a backend ML model and a real-time web dashboard to detect and forecast potential flooding events using environmental parameters like water level, temperature, and rainfall.

---

##  Hardware Setup

The system reads environmental data via:

- **HW-038 Water Level Sensor**
- **DHT11 Temperature Sensor**
- **Rainfall Data from weather API** 

These data are sent to an **ESP8266** which sends HTTP POST requests with readings to the Flask server (`/predict` endpoint).

---

##  Backend (Flask + ML)

###  `new_app.py`

This script does the following:

- **Loads the trained Randaom Forest Classifier ML model** (`flood_model_demo.pkl`, using `pickle`)
- **Handles sensor data** via a `/predict` POST endpoint
- **Returns prediction**:
  - `0` → No Flood
  - `1` → Flood
- **Estimates flood probability**
- **Categorizes risk**:
  - Low: `< 0.3`
  - Medium: `0.3 - 0.6`
  - High: `0.6 - 0.8`
  - Severe: `> 0.8`
- **Serves dashboard UI** via Flask’s `render_template`
- **Maintains in-memory data history** for visualization via `/data` endpoint

**ML Features Used**:
```python
features = [water_level, temperature, rainfall]
```

Model output:
```json
{
  "prediction": 1,
  "probability": 0.87,
  "risk_level": "Severe",
  "flood_occurred": true
}
```

---

## Frontend Dashboard

### `index.html`

Rendered by Flask, this page provides a live visualization dashboard using:

- **Bootstrap** for layout
- **Chart.js** for graphing:
  - Water Level, Temperature, Rainfall over time
  - Flood Risk Probability trend
- **Risk Indicators** with color-coded severity badges
- **Prediction Status** (Flood or No Flood)
- **Auto-updates** every 5 seconds from `/data` endpoint

---



