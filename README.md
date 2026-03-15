# OpenPrintTag Scanner

## Overview
This project is an ESP32-based NFC scanner and writer designed to work with 3D printer filament spools that follow the [OpenPrintTag](https://openprinttag.org) standard. It allows you to tap a filament spool to the scanner to select it in your printer's management software (e.g., PrusaLink), and automatically deducts the filament used after a print.

The device can be configured via a web interface served over Bluetooth Low Energy (BLE), allowing you to set up WiFi and printer credentials without recompiling the firmware.

More details available on the printables website: https://www.printables.com/model/1581648-openprinttag-scanner 

## Compatibility
Validated by the model author with a Prusa MK4S + MMU3 setup using PrusaLink. The same flow can work with other printers/controllers that expose compatible status and job data.

## How to configure
1.  On first boot, or when WiFi is not configured, the device will start a BLE service.
2.  Connect to the device named "OpenPrintTag" from your computer or phone.
3.  Once connected, you can access a web interface at https://ryanch.github.io/openprinttag_scanner/ to:
    *   Configure WiFi SSID and password.
    *   Set the IP address and API key for your PrusaLink-compatible printer.

## Functionality
*   **NFC Tag Reading/Writing:** Reads and writes data to NFC tags formatted according to the OpenPrintTag specification.
*   **Printer Integration:** Communicates with PrusaLink and OctoPrint to:
    *   Notify the printer when a spool has been selected.
    *   Automatically subtract filament weight from the tag after a print is completed or canceled (canceled prints are estimated from progress).
*   **LCD Display:** Shows the current status of the device, including WiFi connection, NFC scans, and printer status.
*   **Bluetooth Configuration:** Provides a web-based UI over BLE for easy setup.
*   **Extensible Printer Strategy:** The `IPrinterLinkStrategy` interface allows for adding support for other printer control software in the future.
*   **Polling Cadence:** Polls printer state on a short interval (about every 10 seconds in the model description) and syncs spool weight accordingly.

## Hardware Setup

## Supported Hardware Profiles

The firmware now supports multiple hardware configurations using separate **PlatformIO environments** defined in `platformio.ini`.

### 1. Full Scanner (Original Design)

This matches the original OpenPrintTag scanner design.

**Hardware:**

- ESP32 DevKitC (ESP32‑WROOM‑32)
- PN5180 NFC module
- 16x2 I2C LCD display
- Optional SK6812 RGBW status LED

**PlatformIO environment:**

```
esp32dev
```

This build enables the LCD and uses the original PN5180 wiring described later in this document.

---

### 2. Compact Scanner (ESP32‑S3‑Zero)

A smaller hardware profile designed for compact builds.

**Hardware:**

- ESP32‑S3‑Zero
- PN5180 NFC module
- Optional single SK6812 RGBW LED
- **No LCD required**

This version is intended for small enclosures where a display is unnecessary. Status feedback is provided through the LED and serial logs.

**PlatformIO environment:**

```
esp32-s3-zero
```

Key differences from the full scanner:

- LCD support disabled
- Status LED enabled by default
- Board‑specific PN5180 pin configuration provided via `platformio.ini`

---

Both hardware profiles use the same firmware codebase and differ only by build configuration.

## Building the Firmware

The project uses **PlatformIO environments** to support different hardware configurations.

### Build the Full Scanner (ESP32 DevKit + LCD)

Use the `esp32dev` environment.

Example using PlatformIO CLI:

```
pio run -e esp32dev
```

Or select the **esp32dev** environment in the PlatformIO extension inside VS Code and click **Build**.

### Build the Compact Scanner (ESP32‑S3‑Zero)

Use the `esp32-s3-zero` environment.

Example using PlatformIO CLI:

```
pio run -e esp32-s3-zero
```

Or select the **esp32-s3-zero** environment in the PlatformIO extension inside VS Code and click **Build**.

These environments automatically configure:

- LCD support
- Status LED behavior
- PN5180 pin mappings
- Board‑specific settings

No source code changes are required when switching hardware profiles.

## ESP32‑S3‑Zero PN5180 Wiring

When building the **compact scanner** using the `esp32-s3-zero` PlatformIO environment, the PN5180 module should be wired to the ESP32‑S3‑Zero as follows:

| PN5180 Pin | ESP32‑S3‑Zero Pin | Notes |
|------------|------------------|------|
| RST | GPIO4 | Hardware reset |
| NSS | GPIO5 | SPI chip select |
| MOSI | GPIO6 | SPI data to PN5180 |
| MISO | GPIO7 | SPI data from PN5180 |
| SCK | GPIO8 | SPI clock |
| BUSY | GPIO9 | Flow control |
| GPIO | GPIO10 | Card detection (future use) |
| IRQ | GPIO11 | Interrupt signal |
| AUX | GPIO12 | Auxiliary monitoring |
| VIN | 5V | Power |
| GND | GND | Ground |

> **Note:** These pin assignments are configured through the `platformio.ini` build flags for the `esp32-s3-zero` environment and may be adjusted if your hardware layout requires different pins.

## Hardware Needed
*   NFC Reader/Writer: PN5180 NFC module (ISO 15693)
*   ESP32 Option 1: [ESP32 DevKitC V4](https://a.co/d/gW3zBIJ) (full scanner build)
*   ESP32 Option 2: ESP32‑S3‑Zero (compact scanner build, no LCD required)
*   16x2 I2C LCD module | 1 | 
*   USB Cable: USB-A to USB-C (1)
*   Jumper wires: male-to-female Dupont wires (9)
*   Optional Status LED: SK6812 RGBW (WS2812-compatible addressable LED, 1 pixel)

## Optional Status LED

The scanner firmware now supports an optional single‑pixel addressable status LED. This LED provides visual feedback for scanner state and spool/tag events.

**Recommended LED:**

* SK6812 RGBW (WS2812‑compatible addressable LED)

The firmware is configured for **SK6812 RGBW timing and color order** using:

`NEO_GRBW + NEO_KHZ800`

### Wiring

Default LED data pin:

GPIO4 (ESP32 DevKit default)  
GPIO21 (ESP32‑S3‑Zero onboard LED)

> **Note:** A single SK6812 RGBW LED module is recommended. Many small breakout boards include the necessary capacitor and resistor already. If using a bare LED, a ~330Ω resistor on the data line is recommended for signal stability.

Typical wiring for a single LED:

| LED Pin | ESP32 Pin |
|-------|-----------|
| VCC | 5V |
| GND | GND |
| DIN | GPIO4 |

A common ground between the ESP32 and the LED is required.

### LED Status States

| Event | LED Behavior |
|------|--------------|
| Booting | White |
| WiFi connected | Cyan |
| Ready | Blue |
| Tag detected | Yellow flash |
| Valid spool/tag | Filament color |
| Blank/invalid tag | Red flash |
| Tag removed | Off |
| Write success | Green flash, then firmware restores filament color |
| Write failure | Red flash, then firmware restores filament color or turns LED off |

### Enabling the LED

The status LED feature is **optional** and is controlled via the build configuration in `platformio.ini`.

To enable the LED, add the following build flags:

```
build_flags =
    -DUSE_STATUS_LED=1
    -DSTATUS_LED_PIN=4
```

**Explanation:**

* `USE_STATUS_LED` enables compilation of the LED feature.
* `STATUS_LED_PIN` sets the ESP32 GPIO pin used for the LED data line.

You may change the pin to any suitable GPIO supported by your ESP32 board. For example:

```
build_flags =
    -DUSE_STATUS_LED=1
    -DSTATUS_LED_PIN=16
```

### Disabling the LED

If you do **not** have the optional status LED installed, simply omit the `USE_STATUS_LED` flag from `platformio.ini`.

Example with LED disabled:

```
build_flags =
```

When the flag is not present, the LED code is **not compiled**, and the firmware behaves exactly like the original scanner firmware with no LED functionality.

### Printables BOM (non-printed parts)
The model page lists this BOM:

| Item | Qty | 
|---|---:|---|
| PN5180 NFC module | 1 | 
| ESP32-WROOM-32 (40 pin) | 1 |
| 16x2 I2C LCD module | 1 | 
| USB-A to USB-C cable | 1 | 
| Dupont jumper wires (M/F) | 9 | 

## Hardware Configuration
Connect the components to the ESP32 as follows:

**16x2 I2C LCD (I2C):**
*   **GND:** GND
*   **VCC:** 5V
*   **SDA:** GPIO 23
*   **SCL:** GPIO 22

**PN5180 NFC Module (SPI, right side of ESP32 top to bottom, skipping D12):**

| PN5180 Pin | ESP32 Pin | Direction | Notes |
|------------|-----------|-----------|-------|
| RST        | D13       | Output    | Hardware reset (active low) |
| *(skip)*   | *D12*     | *—*       | *Strapping pin, skip* |
| NSS        | D14       | Output    | SPI chip select (active low) |
| MOSI       | D27       | Output    | SPI data to PN5180 |
| MISO       | D26       | Input     | SPI data from PN5180 |
| SCK        | D25       | Output    | SPI clock |
| BUSY       | D33       | Input     | SPI flow control |
| GPIO       | D32       | Input     | Card detection (future use) |
| IRQ        | D35       | Input     | Interrupt, active HIGH (input-only pin, no pull-up) |
| AUX        | D34       | Input     | Auxiliary monitoring (input-only pin, no pull-up) |
| REQ        | —         | —         | **Not connected.** Only needed for PN5180 firmware updates. |
| VIN        | 5V        | Power     | |
| GND        | GND       | Power     | |

> **Note:** D35 and D34 are input-only pins on the ESP32 (no internal pull-up). D12 is a strapping pin and is skipped.

# Specs Referenced:
*   Python Example: https://github.com/prusa3d/OpenPrintTag/blob/main/utils/rec_update.py
*   PN5180 Datasheet: https://www.nxp.com/docs/en/data-sheet/PN5180A0xx-C3.pdf
*   PrusaLink API Docs: https://hexdocs.pm/prusa_link/PrusaLink.Api.html
*   https://openprinttag.org/generator/ 
*   Spoolman API: https://donkie.github.io/Spoolman/#tag/filament/operation/Find_filaments_filament_get 
