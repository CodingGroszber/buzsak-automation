# buzsak-automation — Garden IS ESP32 PLC 14 IO

Firmware for an **Industrial Shields ESP32 PLC 14 IO** (`14iosplc_ma`) used as a garden/home automation controller. Built with [PlatformIO](https://platformio.org/) on the Arduino framework.

> **Hardware reference:** [Industrial Shields ESP32 PLC 14 I/O product page](https://www.industrialshields.com/shop/esp32-plc-14-11712)  
> **Pin source:** `framework-industrialshields-esp32/variants/14iosplc/pins_is.h`

---

## Wired hardware (this installation)

| Component | Details |
|-----------|---------|
| Power supply | Mean Well HDR-60-24, 24 V / 2.5 A |
| Well pump relay | Finder 40.52.9.024.0000 coil → `Q0_0` |
| Tank pump relay | Finder 40.52.9.024.0000 coil → `Q0_1` |
| 3-state selector switch LED | +V → `Q0_2`, −V → GND |
| Switch right contact (NC) | `I0_0` → GND (active LOW) |
| Switch left contact (NC) | `I0_1` → GND (active LOW) |
| WiFi status LED | `Q0_3` (blinks while connecting, solid = linked) |

---

## PLC program — 3-state switch logic

The selector switch drives three operating modes, evaluated every 50 ms scan cycle.

| Switch position | `I0_0` | `I0_1` | Switch LED (`Q0_2`) | Well pump (`Q0_0`) | Tank pump (`Q0_1`) |
|----------------|--------|--------|---------------------|--------------------|-------------------|
| **Neutral** (middle) | HIGH | HIGH | OFF | OFF | OFF |
| **Right** | LOW | HIGH | Solid ON | 3 s ON / 3 s OFF | 5 s ON / 5 s OFF |
| **Left** | HIGH | LOW | Blink 0.5 s | Continuously ON | OFF |

> Both inputs active simultaneously → Right takes priority.  
> On any mode transition the pump timers reset: Right mode starts with both pumps OFF; Left mode turns the well pump ON immediately.

Serial monitor output on state changes:
```
[WELL] ON / [WELL] OFF   — well pump toggled (Right mode)
[TANK] ON / [TANK] OFF   — tank pump toggled (Right mode)
```

---

## Hardware — Pin Reference

The ESP32 PLC 14 IO has **14 total I/Os**: 9 inputs + 4 transistor outputs + 1 relay output.  
Digital I/Os are managed through an on-board **MCP23017 I2C GPIO expander** (address prefix `0x20xx`); analog inputs are wired to direct ESP32 ADC pins.

### Digital inputs (I0_x) — 0–30 V, HIGH ≥ 10 V

| Symbol | Internal address | Notes |
|--------|-----------------|-------|
| `I0_0` | `0x200B` (MCP23017) | Switch right contact |
| `I0_1` | `0x200A` (MCP23017) | Switch left contact |
| `I0_2` | `0x200C` (MCP23017) | Available |
| `I0_3` | `0x200D` (MCP23017) | Available |
| `I0_4` | `0x2004` (MCP23017) | Available |
| `I0_5` | `0x2006` (MCP23017) | Available |
| `I0_6` | `0x2005` (MCP23017) | Available |

### Analog inputs (AI0_x) — `_MA` variant = 4–20 mA

| Symbol | GPIO | Alias |
|--------|------|-------|
| `I0_7` / `AI0_0` | GPIO 32 | 4–20 mA channel 0 |
| `I0_8` / `AI0_1` | GPIO 33 | 4–20 mA channel 1 |

> On the `_V` variant these same pins carry 0–10 V instead of 4–20 mA.

### Transistor outputs (Q0_x) — 5 V or 24 V selectable

| Symbol | GPIO | Mode-select pin | This install |
|--------|------|----------------|-------------|
| `Q0_0` | GPIO 12 | `S0_24v` (`0x200F`) | Well pump relay coil (24 V) |
| `Q0_1` | GPIO 25 | `S1_24v` (`0x200E`) | Tank pump relay coil (24 V) |
| `Q0_2` | GPIO 26 | `S2_24v` (`0x2001`) | Switch LED (24 V) |
| `Q0_3` | GPIO 27 | `S3_24v` (`0x2000`) | WiFi status LED (24 V) |

Set the mode pin **before** writing to the output:
```cpp
pinMode(S0_24v, OUTPUT);
digitalWrite(S0_24v, HIGH);   // select 24 V
pinMode(Q0_0, OUTPUT);
digitalWrite(Q0_0, LOW);      // output OFF
```

### Relay output

| Symbol | Internal address |
|--------|-----------------|
| `R0_0` / `REL` | `0x2007` (MCP23017) |

### Communications

| Interface | Details |
|-----------|---------|
| WiFi | IEEE 802.11 b/g/n (ESP32 built-in) |
| Bluetooth | BT 4.2 / BLE (ESP32 built-in) |
| Ethernet | SPI, CS on GPIO 14 (`PIN_SPI_SS_ETHERNET_LIB`) |
| RS-485 | UART 2, DE/RE on GPIO 5 (`RS485_DE_RE`) |
| I²C | SDA = GPIO 21, SCL = GPIO 22 |
| SPI | MOSI = GPIO 23, MISO = GPIO 19, SCK = GPIO 18 |

---

## Features

- **WiFi + OTA** — connects to the configured network on boot; supports over-the-air firmware updates via ArduinoOTA and [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) (`/update`)
- **REST API** — lightweight JSON endpoints consumed by a companion app or Flask server
  - `GET /api/state` — full IO snapshot (firmware version, IP, all input & output states)
  - `POST /api/control` — override any `controllable` output (`name=<key>&state=<1|0>`)
- **Built-in status page** — `GET /` serves a dark-theme HTML dashboard that auto-refreshes every 2 seconds
- **WiFi watchdog** — automatically reconnects if the connection drops

---

## Project structure

```
src/
  main.cpp          # IO layout (INPUTS[], OUTPUTS[]), setup(), loop()
  wifi_network.cpp  # WiFi, HTTP handlers, ArduinoOTA, ElegantOTA
  logic.cpp         # PLC scan cycle — the actual automation program
include/
  io_config.h       # Shared IO structs, extern declarations, FIRMWARE_VERSION
  wifi_network.h    # Public API of wifi_network.cpp
  logic.h           # Public API of logic.cpp
platformio.ini      # PlatformIO build configuration
```

### Adding / removing IO

Edit the `INPUTS[]` and `OUTPUTS[]` arrays in `src/main.cpp` — every other file picks up the change automatically through `include/io_config.h`.

### Writing PLC logic

Add ladder rungs or sequential logic inside `plcLoop()` in `src/logic.cpp`. The function is called every 50 ms from `loop()`.

---

## Configuration

| Setting | Location | Default |
|---|---|---|
| WiFi SSID / password | `src/wifi_network.cpp` | `neunet` / `****` |
| Firmware version | `include/io_config.h` | `v4.0` |
| OTA hostname | `src/wifi_network.cpp` | `PLC-14IO-Cabinet1` |
| OTA password | `src/wifi_network.cpp` | `admin` |
| Upload port | `platformio.ini` | `COM3` |

---

## Dependencies

| Library | Source |
|---|---|
| `ElegantOTA` v3 | [ayushsharma82/ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) |
| Industrial Shields ESP32 platform | [platform-industrialshields-esp32](https://github.com/Industrial-Shields/platform-industrialshields-esp32) |

---

## Build & upload

```bash
# Build
pio run

# Upload over USB
pio run --target upload

# Upload over WiFi (after first USB flash)
pio run --target upload --upload-port <IP_ADDRESS>

# Open serial monitor
pio device monitor
```
## Circut Diagram (placeholder)
```tikz
\usepackage{circuitikz}
\begin{document}
\begin{circuitikz}
  \draw (0,0) to[battery1, l=$V_s$] (0,3)
              to[R=$R_1$] (3,3)
              to[C=$C_1$] (3,0) -- (0,0);
\end{circuitikz}
\end{document}
```
