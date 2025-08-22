## Light Up Your Life (ESP8266 + TCS34725)

A multi-part IoT lab using ESP8266, Adafruit TCS34725 color/light sensor, LEDs, I2C LCD, and a rotary encoder to visualize ambient light levels and interactively tune thresholds.

Watch the demo video: [YouTube](https://youtu.be/NDT4jKHE5MQ)

### Hardware

- ESP8266 (NodeMCU-style)
- Adafruit TCS34725 RGB/clear light sensor (I2C)
- 16x2 I2C LCD (address 0x27)
- 3 x LEDs (red, yellow, green) + resistors
- 1 x momentary button
- 1 x rotary encoder with push button
- Jumper wires, breadboard

I2C bus (shared by TCS34725 and LCD):
- SCL: GPIO5 (D1)
- SDA: GPIO4 (D2)

Other connections (as used in sketches):
- Sensor LED control (`TCS_LED`): GPIO10 (FullAssembly/Part1/Part3) or GPIO0 (Part2)
- Built-in ESP LED: GPIO2 (D4) (Part3)
- Status LEDs: red=GPIO12 (D6), yellow=GPIO13 (D7), green=GPIO15 (D8)
- LCD: I2C @ 0x27 (SCL/SDA as above)
- User button (LCD/part switch): GPIO14 (D5) with INPUT_PULLUP
- Rotary encoder: CLK=GPIO2 (D4), DT=GPIO0 (D3), SW=GPIO9 (encoder push, INPUT_PULLUP)

Note: Pin choices reflect the provided sketches. On some ESP8266 boards, GPIO9/10 are SDIO pins and may be constrained. If your board does not expose them, remap to available GPIOs and update the constants accordingly.

### Project Structure

- `ArduinoSketches/Part1/Part1.ino`: Average lux over a window; drive sensor LED above threshold
- `ArduinoSketches/Part2/Part2.ino`: Classify lux into LOW/MEDIUM/HIGH; drive RGB LEDs; average window drives sensor LED
- `ArduinoSketches/Part3/Part3.ino`: Adds moving average and LED flashing rate proportional to lux; uses built-in LED to reflect average
- `ArduinoSketches/FullAssembly/FullAssembly.ino`: Integrates LCD, encoder, button to switch modes and adjust thresholds live

### Procedure

Common setup for all parts:
1. Wire I2C SCL to GPIO5 (D1), SDA to GPIO4 (D2). Power TCS34725 with 3.3V and GND.
2. If using external LEDs, connect each via a resistor to GPIO12/13/15, with common ground.
3. If using LCD, connect to the same I2C bus (address 0x27).
4. Wire the user button from GPIO14 to GND; enable `INPUT_PULLUP` in code.
5. Wire encoder CLK to GPIO2, DT to GPIO0, and SW to GPIO9 with pull-ups.

Per sketch:
- Part 1 (`Part1.ino`):
  - Window: 5 s, sample every 1 s. Threshold: 40 lux.
  - Action: Sensor LED (`TCS_LED`) turns ON if average lux ≥ threshold.
  - Observe: Serial prints Lux and Avg Lux.

- Part 2 (`Part2.ino`):
  - Thresholds: LOW ≤ 20, HIGH ≥ 70 (MEDIUM in between).
  - Actions: Green/Yellow/Red LEDs indicate LOW/MEDIUM/HIGH. Average window still controls `TCS_LED` at 40 lux.
  - Observe: Serial prints Lux and level (LOW/MEDIUM/HIGH).

- Part 3 (`Part3.ino`):
  - Adds moving average over 5 s; computes LED flash interval linearly between two points: (lux=20 → 1050 ms) and (lux=150 → 10 ms).
  - Actions: Built-in LED reflects average threshold crossing; `TCS_LED` flashes faster with higher lux when moving average > threshold.
  - Observe: Serial prints window, thresholds, Lux, Avg Lux, Moving Avg, Flash interval.

- Full Assembly (`FullAssembly.ino`):
  - LCD shows mode, window, threshold, lux, moving average, and flash interval.
  - Button on GPIO14 cycles Part 1 → 2 → 3.
  - Encoder rotates to adjust `Y_threshold` (0–100) and press to reset to base value (20).
  - Actions from Part 1–3 apply as above, with live threshold tuning.

### Expected Results

- Increasing ambient light raises measured lux. When the average over the 5 s window exceeds the configured threshold, indicator LEDs and/or the built-in LED respond accordingly.
- In Part 2, the color of the external LEDs changes to indicate LOW/MEDIUM/HIGH ranges.
- In Part 3 and Full Assembly, the sensor LED flash rate decreases (faster flashing) as lux increases, following a linear mapping between the defined points.
- The LCD in Full Assembly provides at-a-glance status, and the encoder enables on-the-fly threshold adjustment.

### Build & Upload

1. Open the desired `*.ino` in Arduino IDE (ESP8266 board package installed).
2. Select your ESP8266 board and COM port.
3. Install required libraries:
   - Adafruit TCS34725
   - Adafruit BusIO (dependency providing `Adafruit_I2CDevice.h`)
   - LiquidCrystal_I2C
4. Upload and open Serial Monitor at 115200 baud.
