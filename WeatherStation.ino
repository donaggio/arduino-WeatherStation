/* Wemos D1 Mini Home Weather Station
 * 
 * Depends on the following Arduino libraries:
 * - Adafruit Unified Sensor Library: https://github.com/adafruit/Adafruit_Sensor
 * - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
 * - BMP085 Unified Sensor Library: https://github.com/adafruit/Adafruit_BMP085_Unified
 * 
 * Connection D0 <--> RST is required to wake-up from deep sleep mode
 */

// External libraries
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <Adafruit_BMP085_U.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

// Credentials
#include "auth.h"

// DHT sensor-specific definitions
#define DHTPIN      D4    // Pin connected to sensor
#define DHTTYPE     DHT11 // Sensor type (DHT11, DHT22, DHT21)

void setup() {
  // Objects
  DHT_Unified dht(DHTPIN, DHTTYPE, 6, 10011, 20011);
  Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(30180);
  ESP8266WiFiMulti WiFiMulti;
  HTTPClient http;
  
  // Variables
  sensor_t sensor;
  sensors_event_t event;
  uint32_t sensorDelayMS;
  uint16_t numReadings = 10;
  uint16_t i = 0;
  float temperature = 0.0;
  float humidity = 0.0;
  float pressure = 0.0;
  uint16_t elapsedWiFiConnTimeMS;
  uint16_t maxWiFiConnTimeMS = 30000;
  String host = "192.168.1.77";
  uint16_t port = 8901;
  String baseUrl = "/api/sensors/v1/addSensorsData";
  String url;
  int httpCode;
  uint32_t deepSleepS = (5 * 60);

  // Initialize serial interface output
  Serial.begin(9600);
  
  // Connect D0 to RST to wake up from deep sleep mode
  pinMode(D0, WAKEUP_PULLUP);

  Serial.println();
  Serial.println("Waking up!");
  Serial.println();
  Serial.println("Connected Sensors details");
  Serial.println();
  
  // Initialize DHT device
  dht.begin();
  // Print temperature sensor details.
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  Serial.println();
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
  Serial.println();
  // Get minimum delay between sensor readings
  sensorDelayMS = (sensor.min_delay / 1000);

  // Initialize BMP device
  if (!bmp.begin()) {
    Serial.print("No BMPxxx detected... Aborting!");
    while(1);
  }
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Pressure");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");  
  Serial.println("------------------------------------");
  Serial.println();
  // Get minimum delay between sensor readings
  if ((sensor.min_delay / 1000) > sensorDelayMS) sensorDelayMS = (sensor.min_delay / 1000);

  // Get sensors data
  Serial.println("------------------------------------");
  while (i < numReadings) {
    delay(sensorDelayMS);
    Serial.printf("Reading #%u:\n", ++i);
    // Get temperature event and print its value
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println("Error reading temperature!");
    } else {
      temperature = event.temperature;
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" *C");
    }
    // Get humidity event and print its value
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println("Error reading humidity!");
    } else {
      humidity = event.relative_humidity;
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("%");
    }
    // Get pressure event and print its value
    bmp.getEvent(&event);
    if (isnan(event.pressure)) {
      Serial.println("Error reading pressure!");
    } else {
      pressure = event.pressure;
      Serial.print("Pressure: ");
      Serial.print(pressure);
      Serial.println(" hPa");
    }
  }
  // Connect to local WiFi network
  Serial.println();
  Serial.println("Connecting to local WiFi network...");
  elapsedWiFiConnTimeMS = 0;
  WiFiMulti.addAP(WIFI_SSID1, WIFI_PASSPHRASE1);
  WiFiMulti.addAP(WIFI_SSID2, WIFI_PASSPHRASE2);
  while ((WiFiMulti.run() != WL_CONNECTED) && (elapsedWiFiConnTimeMS < maxWiFiConnTimeMS)) {
    Serial.print(".");
    delay(500);
    elapsedWiFiConnTimeMS += 500;
  }
  Serial.println();
  if ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.println("Failed! Skipping sensors data transmission to remote host.");
  } else {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Transmit sensors data to remote server
    Serial.println();
    Serial.printf("Sending sensors data to %s:%u...\n", host.c_str(), port);
    url = baseUrl + "?authToken=" + API_AUTHTOKEN;
    url += "&t=" + String(temperature, 2);
    url += "&h=" + String(humidity, 2);
    url += "&p=" + String(pressure, 2);
    http.begin(host, port, url);
    httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("Done! Server responded with code %u.\n", httpCode);
    } else {
      Serial.printf("Failed! Error: %s.\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }

  // Enter deep sleep
  Serial.println();
  Serial.println("Going back to sleep!");
  ESP.deepSleep(deepSleepS * 1000000);
}

void loop() {
}
