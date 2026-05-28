// wetter — ESP32-C3 Super Mini
// DHT11 temp/humidity + reed switch anemometer → Home Assistant via MQTT

// Libraries required:
// - DHT sensor library (Adafruit)
// - Adafruit Unified Sensor
// - ArduinoHA

#include "DHT.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoHA.h>
#include "config.h"

#define DHTPIN               4
#define REED_PIN             5

#define ANEMOMETER_RADIUS_M  0.074   // pivot to cup center, meters
#define WIND_CAL_FACTOR      2.5     // adjust after calibration
#define PULSES_PER_REV       2       // 2 magnets on rotor

#define WIND_INTERVAL        5000    // ms between wind publishes
#define DHT_INTERVAL         60000   // ms between temp/humidity publishes

// --- Reed switch interrupt ---

volatile uint32_t pulseCount = 0;
volatile unsigned long lastPulseTime = 0;

void IRAM_ATTR onPulse() {
    unsigned long now = millis();
    if (now - lastPulseTime > 15) {  // 15ms debounce
        pulseCount++;
        lastPulseTime = now;
    }
}

// --- Sensors & MQTT ---

DHT dht(DHTPIN, DHT11);
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

HASensorNumber sensorTemp("wetter_temp", HASensorNumber::PrecisionP1);
HASensorNumber sensorHumidity("wetter_humidity", HASensorNumber::PrecisionP1);
HASensorNumber sensorWindMs("wetter_wind_ms", HASensorNumber::PrecisionP1);
HASensorNumber sensorWindKmh("wetter_wind_kmh", HASensorNumber::PrecisionP1);

unsigned long lastWindPublish = 0;
unsigned long lastWindRead    = 0;
unsigned long lastDhtPublish  = 0;

void onMqttConnected() {
    Serial.println("Connected to broker!");
}

// --- Setup ---

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Wetter starting...");

    pinMode(REED_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(REED_PIN), onPulse, FALLING);
    Serial.println("Reed switch ready");

    dht.begin();

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setHostname("wetter");
    WiFi.setTxPower(WIFI_POWER_8_5dBm);  // C3 Super Mini antenna workaround
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.printf("Connected: %s\n", WiFi.localIP().toString().c_str());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    byte mac[6];
    WiFi.macAddress(mac);
    device.setUniqueId(mac, sizeof(mac));
    device.setName("Wetter");
    device.setSoftwareVersion("1.0.0");

    mqtt.onConnected(onMqttConnected);

    sensorTemp.setName("Temperatur");
    sensorTemp.setDeviceClass("temperature");
    sensorTemp.setUnitOfMeasurement("°C");

    sensorHumidity.setName("Luftfeuchtigkeit");
    sensorHumidity.setDeviceClass("humidity");
    sensorHumidity.setUnitOfMeasurement("%");

    sensorWindMs.setName("Windgeschwindigkeit m/s");
    sensorWindMs.setUnitOfMeasurement("m/s");

    sensorWindKmh.setName("Windgeschwindigkeit km/h");
    sensorWindKmh.setDeviceClass("wind_speed");
    sensorWindKmh.setUnitOfMeasurement("km/h");

    mqtt.begin(BROKER_ADDR, MQTT_USER, MQTT_PASSWORD);

    ArduinoOTA.setHostname("wetter");
    ArduinoOTA.onStart([]() { Serial.println("OTA start"); });
    ArduinoOTA.onEnd([]() { Serial.println("OTA end"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA: %u%%\r", progress / (total / 100));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA error[%u]\n", error);
    });
    ArduinoOTA.begin();

    lastWindRead = millis();
}

// --- Loop ---

void loop() {
    ArduinoOTA.handle();
    mqtt.loop();

    unsigned long now = millis();

    // Wind speed — every WIND_INTERVAL ms
    if (now - lastWindPublish >= WIND_INTERVAL || lastWindPublish == 0) {
        uint32_t pulses    = pulseCount;
        pulseCount         = 0;
        float interval_s   = (now - lastWindRead) / 1000.0f;
        lastWindRead       = now;
        lastWindPublish    = now;

        float rotPerSec    = pulses / (PULSES_PER_REV * interval_s);
        float windSpeedMs  = rotPerSec * (2.0 * PI * ANEMOMETER_RADIUS_M) * WIND_CAL_FACTOR;
        float windSpeedKmh = windSpeedMs * 3.6f;

        Serial.printf("Pulses: %u  Wind: %.1f m/s  %.1f km/h\n", pulses, windSpeedMs, windSpeedKmh);
        sensorWindMs.setValue(windSpeedMs);
        sensorWindKmh.setValue(windSpeedKmh);
    }

    // Temperature + humidity — every DHT_INTERVAL ms
    if (now - lastDhtPublish >= DHT_INTERVAL || lastDhtPublish == 0) {
        lastDhtPublish = now;

        float h = dht.readHumidity();
        float t = dht.readTemperature();

        if (isnan(h) || isnan(t)) {
            Serial.println("DHT read failed!");
        } else {
            Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n", t, h);
            sensorTemp.setValue(t);
            sensorHumidity.setValue(h);
        }
    }
}
