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
| Pressure sensor | TOSEASTARS PT506, 0–10 bar, 4–20 mA → Red (IN+) to 24V+, Black (IN−) to `I0_7` |
| Water level sensor | YWBL-WH TL-136, 0–1 m, 4–20 mA → Red (IN+) to 24V+, Black (IN−) to `I0_8` |

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
- **Analog probe diagnostic** — build with `-DDIAG_ENABLED=1` to expose `/diag` (live HTML), `/diag.csv` (download), `/diag/clear`; samples GPIO 32/33 raw + calibrated + mV + derived current/voltage every 2 s and logs to LittleFS

---

## Project structure

```
src/
  main.cpp          # IO layout (INPUTS[], OUTPUTS[], ANALOG_INPUTS[]), setup(), loop()
  wifi_network.cpp  # WiFi, HTTP handlers, ArduinoOTA, ElegantOTA
  logic.cpp         # PLC scan cycle — the actual automation program
  diag.cpp          # Analog probe diagnostic (compiled in only when DIAG_ENABLED=1)
include/
  io_config.h       # Shared IO structs, extern declarations, FIRMWARE_VERSION
  wifi_network.h    # Public API of wifi_network.cpp
  logic.h           # Public API of logic.cpp
  diag.h            # Diagnostic config macros and diagSetup()/diagLoop() declarations
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
| Upload port | `platformio.ini` | `COM3` (USB) / `192.168.1.94` (OTA) |
| Diagnostic enable | `platformio.ini` `build_flags` | `DIAG_ENABLED=1` (currently on) |
| Shunt resistance | `include/diag.h` `DIAG_SHUNT_OHMS` | `165.0` Ω |
| Voltage divider | `include/diag.h` `DIAG_V_DIVIDER` | `3.0303` |
| Diag sample interval | `include/diag.h` `DIAG_INTERVAL_MS` | `2000` ms |
| Diag CSV size cap | `include/diag.h` `DIAG_MAX_BYTES` | `512 KB` |

---

## Known issues / in progress

| # | Issue | Status |
|---|-------|--------|
| 1 | **Analog inputs reading invalid values** — pressure (PT506) and water level (TL-136) sensors wired correctly (Red→24V+, Black→`I0_7`/`I0_8`, GND→DC−) but `/diag` not yet readable because OTA upload is blocked by Windows Firewall on the dev machine. Diagnostic firmware is built and committed (`DIAG_ENABLED=1`). | **Open** — flash pending; workaround: allow Python through Windows Firewall then re-run OTA upload to `192.168.1.94`. |
| 2 | **OTA upload fails from this PC** — `espota` authenticates (`OK`) but times out waiting for device response. Root cause: Windows Firewall blocks the inbound TCP connection the ESP32 opens back to the OTA host. | **Open** — fix: `Windows Defender Firewall → Allow an app → python.exe` (private network). |
| 3 | **`DIAG_ENABLED=1` left on in repo** — diagnostic routes and LittleFS logging active in production build; should be set back to `0` once sensor readings are verified. | **Open** |

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

# Upload over WiFi (after first USB flash) — use IP, not hostname (mDNS not resolved by espota)
pio run --target upload --upload-port 192.168.1.94

# Open serial monitor
pio device monitor
```

> **OTA note:** if upload authenticates but then times out, Windows Firewall is blocking the inbound callback from the device. Allow `python.exe` through the firewall (private network) and retry.
## Circut Diagram ( placeholder )
![Circuit Diagram 1](docs/images/circuit_1.svg)
