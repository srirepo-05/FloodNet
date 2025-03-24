#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 14        // Pin for DHT11
#define DHTTYPE DHT11    // DHT 11 Sensor
#define WATER_LEVEL_PIN A0  // Analog pin for HW-038 sensor
#define MIN_ANALOG 15       // Analog value when water is at 0 mm (dry)
#define MAX_ANALOG 415      // Analog value when water is at max height
#define MAX_HEIGHT_MM 20    // Maximum water level height in mm

const char* ssid = "Sri's G51";        // 🔹 Your WiFi Name
const char* password = "hotspotg51*"; // 🔹 Your WiFi Password
const char* serverUrl = "http://192.168.156.98:5000/predict"; // 🔹 Flask Server URL

DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    
    dht.begin();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        // 🔹 Read Temperature from DHT11 Sensor
        float temperature = dht.readTemperature();
        if (isnan(temperature)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        // 🔹 Read Water Level from HW-038 Sensor
        int sensorValue = analogRead(WATER_LEVEL_PIN);
        float waterLevelMM = map(sensorValue, MIN_ANALOG, MAX_ANALOG, 0, MAX_HEIGHT_MM);
        waterLevelMM = constrain(waterLevelMM, 0, MAX_HEIGHT_MM);  // Ensure valid range

        // 🔹 Get Rainfall Data from Weather API
        float precipitation = getRainfallFromAPI();  // Function call to fetch API data

        // 🔹 Create JSON Payload
        StaticJsonDocument<200> doc;
        doc["Water Level"] = waterLevelMM;
        doc["Temperature"] = temperature;
        doc["Rainfall"] = precipitation;

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.println("Sending Data: " + jsonPayload);

        // 🔹 Send Data to Flask Server
        http.begin(client, serverUrl);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(jsonPayload);
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Server Response: " + response);
        } else {
            Serial.print("Error in HTTP request: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }

    delay(3000); // 🔹 Send data every 30 seconds
}


float getRainfallFromAPI() {
    WiFiClient client;
    HTTPClient http;

    String apiKey = "95bb414b6da04c7dade64954251203";
    String city = "Kasongan";
    String url = "http://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city;

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            return doc["current"]["precip_mm"].as<float>();  // Extract Rainfall
        } else {
            Serial.println("JSON Parsing Error");
            return -1;
        }
    } else {
        Serial.println("Failed to fetch rainfall data");
        return -1;
    }
    http.end();
}

