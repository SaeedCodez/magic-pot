# Magic Pot 🪴

A smart plant monitor built on the ESP8266. It reads soil moisture and shows a face on a small OLED screen — happy when the soil is good, sad when it is too dry or too wet. When the plant gets dry, a short melody plays as an alert.

---

## How it works

The soil moisture sensor sends a value from 0 to 100%.

| Moisture  | State | Face                                    | Buzzer                        |
|-----------|-------|-----------------------------------------|-------------------------------|
| 0 – 30%   | DRY   | Sad droopy eyes, frown, inverted screen | Plays a melody for 5 seconds  |
| 31 – 69%  | OK    | Closed ^^ eyes, gentle smile            | Silent                        |
| 70 – 100% | WET   | Big shiny eyes, smile, water drops      | Silent                        |

The face blinks every 3.5 seconds regardless of state.

---

## Hardware

| Part               | Details                              |
|--------------------|--------------------------------------|
| Microcontroller    | ESP8266 NodeMCU v3 (CH340)           |
| Display            | SSD1306 OLED 128x64, I2C (0x3C)     |
| Moisture sensor    | Analog output connected to A0        |
| Buzzer             | Connected to GPIO12                  |

### Wiring

```
Moisture sensor AO  -->  A0
Buzzer              -->  GPIO12
OLED SDA            -->  D2 (GPIO4)
OLED SCL            -->  D1 (GPIO5)
OLED VCC            -->  3.3V
OLED GND            -->  GND
```

---

## Libraries required

Install these through the Arduino Library Manager:

- Adafruit SSD1306
- Adafruit GFX Library

---

## Setup

1. Open `main/main.ino` in the Arduino IDE.
2. Select board: **NodeMCU 1.0 (ESP-12E Module)**.
3. Install the libraries listed above.
4. Upload and open the Serial Monitor at **9600 baud** to see live readings.

---

## Adjusting thresholds

At the top of `main.ino`:

```cpp
#define THRESHOLD_DRY 30   // below this = DRY
#define THRESHOLD_WET 70   // above this = WET
```

Change these values to match your plant and soil type.

---

## Serial output

Every second, a line like this is printed:

```
Moisture: 45%  |  Raw: 563  |  Level: OK
```
