#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define DHTPIN 2       // Pin for DHT11
#define DHTTYPE DHT11       // DHT 11 Sensor
#define WATER_LEVEL_PIN A0  // Analog pin for HW-038 sensor
#define MIN_ANALOG 240       // Analog value when water is at 0 mm (dry)
#define MAX_ANALOG 390     // Analog value when water is at max height
#define MAX_HEIGHT_MM 40    // Maximum water level height in mm

// Wi-Fi credentials
const char* ssid = "SNUC";
const char* password = "snu12345";

// Weather API settings
const String apiKey = "95bb414b6da04c7dade64954251203";
const String city = "bontang"; // e.g., "London"

// Flask server URL
const char* flaskServerUrl = "http://10.123.109.165:5000/predict";

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize WiFi client
WiFiClient wifiClient;

// Variables for sensor data
float temperature;
float waterLevel;
float rainfall;

// Time management for sensor readings
unsigned long previousMillis = 0;
const long interval = 5000;  // 15 seconds interval

void setup() {
  Serial.begin(115200);
  delay(100);


  
  // Initialize DHT sensor
  dht.begin();
  
  // OLED setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.display();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check if it's time to read sensors and send data (every 15 seconds)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Read sensor data
    readSensorData();
    
    // Send data to Flask server
    sendDataToServer();
  }
}

void readSensorData() {
  // Read temperature from DHT11
  temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    temperature = 0.0;
  }
  
  // Read water level from HW-038
  int rawWaterLevel = analogRead(WATER_LEVEL_PIN);
  
  // Convert analog value to water level in mm
  // Map the analog value to the range 0 to MAX_HEIGHT_MM
  if (rawWaterLevel <= MIN_ANALOG) {
    waterLevel = 0;
  } else if (rawWaterLevel >= MAX_ANALOG) {
    waterLevel = MAX_HEIGHT_MM;
  } else {
    waterLevel = map(rawWaterLevel, MIN_ANALOG, MAX_ANALOG, 0, MAX_HEIGHT_MM);
  }
  
  // Get rainfall data from Weather API
  getRainfallData();
  
  // Print data for debugging
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Water Level: ");
  Serial.print(waterLevel);
  Serial.print(" mm, Rainfall: ");
  Serial.print(rainfall);
  Serial.println(" mm");


  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Flood Monitoring");
  display.setCursor(0, 15);
  display.print("Temp (C): ");
  display.println(temperature);

  display.setCursor(0, 30);
  display.print("Rain (mm): ");
  display.println(rainfall);

  display.setCursor(0, 45);
  display.print("WaterLvl (mm): ");
  display.println(waterLevel);
  display.display();

}

void getRainfallData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String weatherUrl = "http://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city;
    
    // Use the updated begin method that takes a WiFiClient
    http.begin(wifiClient, weatherUrl);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      
      // Use ArduinoJson v6 without specifying capacity
      DynamicJsonDocument doc(1024); // Use a reasonable size
      
      // Parse JSON
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        // Extract rainfall in mm
        rainfall = doc["current"]["precip_mm"].as<float>();
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        rainfall = 0.0;
      }
    } else {
      Serial.println("Failed to get weather data");
      rainfall = 0.0;
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
    rainfall = 0.0;
  }
}

void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Configure target server and URL with WiFiClient
    http.begin(wifiClient, flaskServerUrl);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["temperature"] = temperature;
    jsonDoc["water_level"] = waterLevel;
    jsonDoc["rainfall"] = rainfall;
    
    String jsonPayload;
    serializeJson(jsonDoc, jsonPayload);
    
    // Send POST request
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
