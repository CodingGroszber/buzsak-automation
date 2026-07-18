# buzsak-automation — Garden IS ESP32 PLC 14 IO

Firmware for an **Industrial Shields ESP32 PLC 14 IO** (`14iosplc_ma`) used as a garden/home automation controller. Built with [PlatformIO](https://platformio.org/) on the Arduino framework.

---

## Hardware

| | |
|---|---|
| **Board** | Industrial Shields ESP32 PLC 14 IO (`14iosplc_ma`) |
| **Inputs** | 6 digital inputs — `I0_0` … `I0_5` |
| **Outputs** | 4 relay/transistor outputs — `Q0_0` … `Q0_3` |
| **Output voltage** | Selectable 5 V / 24 V per channel via mode pins `S0_24v`…`S3_24v` |
| **Upload port** | COM3 @ 460 800 baud |
| **Serial monitor** | 115 200 baud |

---

## Features

- **WiFi + OTA** — connects to the configured network on boot; supports over-the-air firmware updates via ArduinoOTA and [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) (`/update`)
- **REST API** — lightweight JSON endpoints consumed by a companion app or Flask server
  - `GET /api/state` — full IO snapshot (firmware version, IP, all input & output states)
  - `POST /api/control` — set any `controllable` output (`name=<key>&state=<1|0>`)
- **Built-in status page** — `GET /` serves a dark-theme HTML dashboard that auto-refreshes every 2 seconds
- **WiFi watchdog** — automatically reconnects if the connection drops
- **WiFi LED** — output `wifi_led` blinks while connecting, stays solid once linked

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

