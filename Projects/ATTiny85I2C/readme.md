# ATTiny85 I2C Slave LED Flasher

An I2C slave running on a Digispark (ATTiny85) that flashes an LED on command.

## Hardware

- Digispark ATTiny85 board (plugs directly into USB)
- LED connected to PB1 (physical pin 6)
- I2C uses PB0 (SDA) and PB2 (SCL)

## I2C Configuration

- **Slave address:** 0x08
- **LED pin:** PB1 (pin 6)

## Usage

Send data from an I2C master to trigger LED flashing:

| Byte | Description |
|------|-------------|
| 1 | Number of times to flash |
| 2 | Flash speed in ms (optional, default 100) |

Reading from the slave returns the remaining flash count (0 = done).

### Arduino Master Example

```cpp
#include <Wire.h>

Wire.begin();
Wire.beginTransmission(0x08);
Wire.write(5);    // flash 5 times
Wire.write(100);  // 100ms on/off delay
Wire.endTransmission();
```

## Driver Setup (Windows)

The Digispark requires libusb drivers on Windows:

1. Download Zadig from https://zadig.akeo.ie/
2. Plug in the Digispark board
3. In Zadig: Options → List All Devices
4. Select the device (may show as "Unknown Device" or "Digispark")
5. Select **libusb-win32** driver and click Install

## Uploading

The Digispark bootloader only runs briefly at power-up:

1. Unplug the board from USB
2. Start upload in PlatformIO (`Ctrl+Alt+U`)
3. Wait for "Please plug in the device" message
4. Plug in the board within 60 seconds

## Wiring

For I2C communication, use 2-4.7k pull-up resistors on SDA and SCL lines.
Pull-ups may be optional for short wires (<10cm) at 100kHz due to R4's internal pull-ups.

### Connections

| ATTiny85 | Arduino R4 |
|----------|------------|
| PB0 (SDA) | A4 (SDA) |
| PB2 (SCL) | A5 (SCL) |
| GND | GND |
| VCC | 5V |

### ATTiny85 Pinout

```
                 ┌──────┐
    (RESET) PB5 ─┤1    8├─ VCC
            PB3 ─┤2    7├─ PB2 (SCL) ──→ A5
            PB4 ─┤3    6├─ PB1 (LED)
            GND ─┤4    5├─ PB0 (SDA) ──→ A4
                 └──────┘
```

### Arduino UNO R4 Pinout (I2C)

```
        USB
    ┌────┴────┐
    │  ┌───┐  │
    │  │   │  │
    │  └───┘  │
    │         │
  ──┤ D13     ├──
  ──┤ D12     ├──
  ──┤ D11     ├──
  ──┤ D10     ├──
  ──┤ D9      ├──
  ──┤ D8      ├──
  ──┤ D7      ├──
  ──┤ D6      ├──
  ──┤ D5   A0 ├──
  ──┤ D4   A1 ├──
  ──┤ D3   A2 ├──
  ──┤ D2   A3 ├──
  ──┤ GND  A4 ├── SDA ←── ATTiny85 PB0
  ──┤ GND  A5 ├── SCL ←── ATTiny85 PB2
  ──┤ 5V  VIN ├──
  ──┤ 3V3 GND ├──
    └─────────┘
```
