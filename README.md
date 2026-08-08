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
| **Right — AUTO** | LOW | HIGH | Solid ON | Water-level hysteresis (see below) | Pressure hysteresis (see below) |
| **Left** | HIGH | LOW | Blink 0.5 s | Continuously ON | OFF |

> Both inputs active simultaneously → Right takes priority.

### AUTO mode — control loops (Right switch)

#### Well pump (`Q0_0`) — water-level hysteresis

| Condition | Well pump |
|-----------|----------|
| Level is low (restart threshold, in liters) | → **ON** |
| Level reaches `WATER_LEVEL_HIGH_L` (full) | → **OFF** |

Control implementation uses liters from `waterLevelMvToLiters(...)`, then applies hysteresis against liter constants.

#### Tank pump (`Q0_1`) — pressure hysteresis

| Condition | Tank pump |
|-----------|----------|
| Pressure drops below **3 bar** | → **ON** (start filling) |
| Pressure reaches **4 bar** | → **OFF** (target reached) |
| Pressure ≤ 0.1 bar (pipe rupture / sensor fault) | → **OFF** (safety stop) |

Serial monitor output on state changes:
```
[WELL] ON   834 L  2000 mV
[WELL] OFF  1000 L 2400 mV
[TANK] ON   2.43 bar   — pump started
[TANK] OFF  4.01 bar   — target reached
```

---

## Usage (day-to-day)

### 1) Normal operation

1. Put selector switch in **Right (AUTO)**.
2. Open status page: `http://192.168.1.94/`.
3. Verify:
  - Pressure card shows **bar** with **mV** sub-value.
  - Water-level card shows **L** with **mV** sub-value.
4. Confirm expected transitions:
  - Well pump starts/stops from level hysteresis.
  - Tank pump starts/stops from pressure hysteresis.

### 2) Manual mode behavior

1. Move selector switch to **Left**.
2. Expected:
  - Well pump forced ON.
  - Tank pump forced OFF.
  - Switch LED blinks every 0.5 s.

### 3) Safe stop

1. Move selector to **Neutral**.
2. Expected:
  - Both pumps OFF.
  - Switch LED OFF.
  - AUTO hysteresis state reset.

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
  - Analog payload includes raw `mv`; pressure channel adds `bar`; water-level channel adds `liters` and `low`
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

| Setting | Location | Value |
|---|---|---|
| WiFi SSID / password | `src/wifi_network.cpp` | Local secret (see `login.md`, gitignored) |
| Firmware version | `include/io_config.h` | `v4.0` |
| OTA hostname | `src/wifi_network.cpp` | `PLC-14IO-Cabinet1` |
| ArduinoOTA password | `src/wifi_network.cpp` | Local secret (see `login.md`, gitignored) |
| ElegantOTA user / password | `src/wifi_network.cpp` | Local secret (see `login.md`, gitignored) |
| Upload target | `platformio.ini` `default_envs` | `ota` (WiFi) or `usb` (COM3) |
| OTA device IP | `platformio.ini` `[env:ota]` | `192.168.1.94` |
| Diagnostic enable | `platformio.ini` `build_flags` | `DIAG_ENABLED=1` (currently on) |
| Shunt resistance | `platformio.ini` `build_flags` | `DIAG_SHUNT_OHMS=470.0` Ω |
| Voltage divider | `platformio.ini` `build_flags` | `DIAG_V_DIVIDER=3.82` (calibrated) |
| Diag sample interval | `include/diag.h` `DIAG_INTERVAL_MS` | `2000` ms |
| Diag CSV size cap | `include/diag.h` `DIAG_MAX_BYTES` | `512 KB` |

---

## Known issues / in progress

| # | Issue | Status |
|---|-------|--------|
| 1 | **`DIAG_ENABLED=1` left on** — diagnostic routes and LittleFS logging active; disable once sensor readings are finalised. | **Open** |
| 2 | **Level sensor (TL-136) over-range** — reading ~22 mA / 10.3 V; tank is at or above sensor's 0–1 m full-scale. Needs re-zeroing or sensor repositioning. | **Open** |

---

## Dependencies

| Library | Source |
|---|---|
| `ElegantOTA` v3 | [ayushsharma82/ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) |
| Industrial Shields ESP32 platform | [platform-industrialshields-esp32](https://github.com/Industrial-Shields/platform-industrialshields-esp32) |

---

## Build & upload

```bash
# Build only (OTA env)
platformio run -e ota

# Build only (USB env)
platformio run -e usb
```

### Upload workflows (recommended)

This project supports two upload paths:

1. **OTA (default env)** — fastest for iterative updates, uses `scripts/http_ota.py`
2. **USB** — recovery path when device is offline or OTA fails

### OTA upload (custom HTTP flow)

`platformio.ini` uses:
- `upload_protocol = custom`
- `upload_command = python $PROJECT_DIR/scripts/http_ota.py $UPLOAD_PORT $SOURCE`

Recommended commands on this machine:

```powershell
# From project root
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e ota --target upload
```

If dashboard HTML changes and the page looks stale, force clean + upload:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e ota -t clean
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e ota --target upload
```

What the OTA script does:

1. `GET /ota/start?mode=firmware&hash=<md5>`
2. `POST /ota/upload` (multipart firmware)
3. Waits for reboot and verifies device comes back online

### USB upload (fallback / recovery)

Use USB when:

1. Device is not reachable at `192.168.1.94`
2. WiFi credentials changed or network is unstable
3. OTA update was interrupted

Steps:

1. Connect USB cable.
2. Confirm COM port (default in this project: `COM3`).
3. Upload:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e usb --target upload
```

### Verify after upload

1. Open `http://192.168.1.94/`.
2. Check firmware/version string in header.
3. Check `GET /api/state` returns expected `bar`, `liters`, and `mv` fields.
4. Confirm switch modes and pump behavior match the truth table above.

### Troubleshooting uploads

1. **`pio` command not found**
  - Use full executable path:
    `"$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"`
2. **OTA succeeds but webpage looks old**
  - Run clean build before upload.
  - Hard refresh browser (`Ctrl+F5`) or open private window.
3. **OTA cannot connect to host**
  - Verify PLC IP in `platformio.ini` (`upload_port = 192.168.1.94`).
  - Ensure PC and PLC are on same subnet.
  - Fallback to USB upload.
4. **Wrong COM port on USB**
  - Update `upload_port` under `[env:usb]` in `platformio.ini`.

> **ElegantOTA (browser alternative):** open `http://192.168.1.94/update`, authenticate with local credentials from `login.md`, then upload `.pio/build/ota/firmware.bin`.

> **ArduinoOTA note:** if you use ArduinoOTA transport instead of the custom HTTP uploader, you may need an inbound firewall rule for PlatformIO Python.
## Circut Diagram ( placeholder )
![Circuit Diagram 1](docs/images/circuit_1.svg)
