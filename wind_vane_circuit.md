# Wind Vane — Resistor Ladder Circuit

8 NO reed switches, 1 ADC pin, 8-direction resolution.

---

## Parts

- 8× NO reed switch
- 1× 10kΩ resistor (pull-up)
- 8× series resistors (see table below) — all standard E12 values
- 1 free ADC-capable GPIO on the ESP32-C3

---

## Circuit

```
3.3V
 │
10kΩ  (pull-up)
 │
 ├──── ADC_PIN (e.g. GPIO3)
 │
 ├── [1kΩ]  ── [Reed N]  ──┐
 ├── [2.2kΩ] ── [Reed NE] ──┤
 ├── [3.3kΩ] ── [Reed E]  ──┤
 ├── [4.7kΩ] ── [Reed SE] ──┤
 ├── [6.8kΩ] ── [Reed S]  ──┤
 ├── [10kΩ]  ── [Reed SW] ──┤
 ├── [15kΩ]  ── [Reed W]  ──┤
 └── [22kΩ]  ── [Reed NW] ──┤
                             │
                            GND
```

Each reed switch connects its series resistor to GND when a magnet is present.
Only one switch is active at a time — one magnet on the vane.

---

## How it works

When switch N (1kΩ) closes:

```
V_adc = 3.3V × 1kΩ / (10kΩ + 1kΩ) = 0.30V → ADC ≈ 372
```

Each resistor gives a unique voltage → unique ADC reading.

---

## Expected ADC readings (12-bit, 0–4095)

| Direction | Resistor | ADC (approx) | Threshold to next |
|-----------|----------|--------------|-------------------|
| N         | 1kΩ      | 372          | < 555             |
| NE        | 2.2kΩ    | 738          | 555 – 877         |
| E         | 3.3kΩ    | 1016         | 877 – 1163        |
| SE        | 4.7kΩ    | 1309         | 1163 – 1483       |
| S         | 6.8kΩ    | 1657         | 1483 – 1853       |
| SW        | 10kΩ     | 2048         | 1853 – 2253       |
| W         | 15kΩ     | 2457         | 2253 – 2636       |
| NW        | 22kΩ     | 2815         | 2636 – 3800       |
| (none)    | —        | ~4095        | > 3800            |

---

## Direction recognition (sketch logic)

Average a few ADC samples, then compare to thresholds:

```cpp
#define VANE_PIN  3   // pick a free ADC pin

int readVane() {
    long sum = 0;
    for (int i = 0; i < 5; i++) { sum += analogRead(VANE_PIN); delay(2); }
    int adc = sum / 5;

    if (adc < 555)  return 0;   // N
    if (adc < 877)  return 1;   // NE
    if (adc < 1163) return 2;   // E
    if (adc < 1483) return 3;   // SE
    if (adc < 1853) return 4;   // S
    if (adc < 2253) return 5;   // SW
    if (adc < 2636) return 6;   // W
    if (adc < 3800) return 7;   // NW
    return -1;                   // no switch active
}

const char* dirName[] = {"N","NE","E","SE","S","SW","W","NW"};
```

---

## Physical installation

The compass assignment (which resistor goes to which physical switch) depends on
how the switches are mounted. During first test:

1. Flash with `Serial.printf("ADC: %d\n", analogRead(VANE_PIN))`
2. Point vane North (or to a known reference), note the ADC value
3. Map that value to the correct resistor/direction
4. Rotate through all 8 positions and verify the table matches

Adjust thresholds if the ESP32-C3 ADC reads slightly off — the gaps between
expected values are ~300–400 counts, so there is plenty of margin.
