# ESP32-S3-Zero Support TODO

Goal: Support a compact scanner build using:

- ESP32-S3-Zero
- PN5180
- No LCD
- Built-in WS2812 LED

This should exist alongside the current hardware configuration (ESP32 DevKit + LCD), not replace it.

---

# Phase 1 — Build Infrastructure

Goal: Make the project compile for multiple hardware configurations.

## Make LCD Optional

Currently `main.cpp` assumes an LCD always exists.

Tasks:

- Add compile flag:
  
  ```
  USE_LCD
  ```

- Wrap LCD includes:

  ```
  #if USE_LCD
  #include "LCDManager.h"
  #endif
  ```

- Wrap LCD initialization and usage in `main.cpp`.

- Allow `ApplicationManager` to run without an LCD pointer.

---

## Add PlatformIO Environment for ESP32-S3-Zero

Add a new environment to `platformio.ini`.

Example:

```
[env:esp32-s3-zero]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

build_flags =
    -DUSE_STATUS_LED=1
    -DSTATUS_LED_PIN=21
```

Notes:

- GPIO21 is the onboard WS2812 LED on the S3-Zero.
- Do not remove the existing `esp32dev` environment.

---

# Phase 2 — Hardware Profile

Goal: Define pin mappings for the S3-Zero build.

Tasks:

- Create board-specific PN5180 pin definitions.
- Avoid hardcoding pins in `main.cpp` where possible.
- Use `build_flags` or header definitions for board variants.

Example concept:

```
PN5180_SCK
PN5180_MISO
PN5180_MOSI
PN5180_SS
PN5180_RST
PN5180_BUSY
PN5180_IRQ
PN5180_GPIO
PN5180_AUX
```

---

# Phase 3 — Built-in LED Support

Goal: Ensure the onboard LED works with the existing LEDManager.

Tasks:

- Confirm `Adafruit_NeoPixel` works correctly on GPIO21.
- Validate color ordering (GRB vs GRBW).
- Verify brightness and color behavior on real hardware.

Possible improvements:

- Add brightness control.
- Adjust white channel usage.

---

# Phase 4 — Remove LCD Dependencies

Goal: Ensure the scanner operates fully without an LCD.

Tasks:

- Replace LCD messages with Serial logging when LCD disabled.
- Ensure BLE configuration flow still works.
- Confirm all boot states behave correctly.

---

# Phase 5 — Hardware Testing

Goal: Validate the S3-Zero build on physical hardware.

Test:

- Boot and initialization
- WiFi connection
- BLE configuration
- NFC tag detection
- Tag writes
- MQTT / Spoolman interaction
- LED state transitions

---

# Phase 6 — Documentation

Update README with a new section:

```
Compact Scanner Build (ESP32-S3-Zero)
```

Include:

- Wiring diagram
- PN5180 connections
- Built-in LED usage
- PlatformIO environment example

---

# Phase 7 — Optional Improvements

Future enhancements:

- Board abstraction layer for hardware variants
- Better LED animations
- Auto-detect hardware profile
- Optional minimal web interface

---

# Long Term Goal

Support two official hardware profiles:

### Full Scanner

```
ESP32 DevKit
PN5180
16x2 LCD
Optional status LED
```

### Compact Scanner

```
ESP32-S3-Zero
PN5180
Built-in LED
No LCD
```

Both builds should share the same firmware with compile-time configuration.