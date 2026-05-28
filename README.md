# Wetter

ESP32-C3 Super Mini weather station built by repurposing the mechanical hardware from an old outdoor weather station unit. The original electronics were discarded; the anemometer cups, wind vane, and reed switches are reused.

<!-- pics coming -->

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32-C3 Super Mini |
| Temperature / Humidity | DHT11 on GPIO4 |
| Anemometer | 3-cup rotor, 2 magnets, 1 reed switch on GPIO5 |
| Wind vane | 8-position, resistor ladder on GPIO3 *(coming — waiting for reed switches)* |

### Anemometer

The original rotor has 2 magnets mounted 180° apart — one magnet, one counterweight. Each full rotation triggers the reed switch twice. Wind speed is calculated from pulse count over a 5-second window:

```
rotations/sec  = pulses / (2 pulses/rev × 5 sec)
tip_speed m/s  = rotations/sec × 2π × 0.074m
wind_speed m/s = tip_speed × 2.5  ← calibration factor, adjust after field test
```

### Wind vane *(coming)*

8 NO reed switches in a resistor ladder — see [`wind_vane_circuit.md`](wind_vane_circuit.md) for wiring and ADC thresholds.

## Home Assistant

Integrates via MQTT auto-discovery (ArduinoHA). Exposes:

- **Temperatur** — °C
- **Luftfeuchtigkeit** — %
- **Windgeschwindigkeit km/h**
- **Windgeschwindigkeit m/s**

OTA updates supported — hostname `wetter`.

## Setup

1. Copy `config.example.h` to `config.h` and fill in your credentials:

```cpp
#define WIFI_SSID      "your-ssid"
#define WIFI_PASSWORD  "your-wifi-password"
#define BROKER_ADDR    IPAddress(192,168,1,1)
#define MQTT_USER      "mqtt"
#define MQTT_PASSWORD  "your-mqtt-password"
```

2. Install libraries in Arduino IDE:
   - DHT sensor library (Adafruit)
   - Adafruit Unified Sensor
   - ArduinoHA

3. Board: **ESP32C3 Dev Module** — flash and open Serial Monitor at 115200.

## Calibration

Once installed, compare wind speed readings against a reference and adjust `WIND_CAL_FACTOR` in `wetter.ino`. The default of `2.5` is a reasonable starting point for a 3-cup anemometer with a 74mm arm.
