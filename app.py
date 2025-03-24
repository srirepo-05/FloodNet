from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import joblib
import numpy as np

app = Flask(__name__)
CORS(app)  # Allow cross-origin requests

# Load the trained model
model = joblib.load("flood_model.pkl")

@app.route("/")
def home():
    return render_template("index.html")  # Serve the dashboard



import pandas as pd  # Import pandas

@app.route('/predict', methods=['POST'])
def predict():
    data = request.get_json()

    # Extract values
    water_level = data["Water Level"]
    temperature = data["Temperature"]
    rainfall = data["Rainfall"]

    # Convert input into a Pandas DataFrame with correct column names
    input_df = pd.DataFrame([[water_level, temperature, rainfall]], 
                            columns=["Water Level", "Temperature", "Rainfall"])

    # Make prediction
    prediction = model.predict(input_df)[0]
    probability = model.predict_proba(input_df)[0][1] * 100  # Convert to percentage

    response = {
        "Flood Prediction": int(prediction),
        "Flood Risk (%)": round(probability, 2)
    }

    return jsonify(response)


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
