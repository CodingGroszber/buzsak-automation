#pragma once

// ═════════════════════════════════════════════════════════════
//  ANALOG PROBE DIAGNOSTIC — isolated from main PLC logic
//
//  Purpose: empirically confirm whether the analog channels behave
//  as 4–20 mA (GPIO 32/33) or 0–10 V (GPIO 34/35) inputs by sampling
//  ALL candidate pins and logging both interpretations to a CSV file.
//
//  Enable by building with -DDIAG_ENABLED=1 (see platformio.ini).
//  When disabled, diagSetup()/diagLoop() compile to nothing.
//
//  Data access (device on WiFi):
//    GET /diag        → live HTML readings
//    GET /diag.csv    → download the accumulated CSV log
//    GET /diag/clear  → wipe the log and rewrite the header
// ═════════════════════════════════════════════════════════════

#ifndef DIAG_ENABLED
#define DIAG_ENABLED 0
#endif

// Sampling period in milliseconds.
#ifndef DIAG_INTERVAL_MS
#define DIAG_INTERVAL_MS 2000
#endif

// Sense-resistor value used to convert measured pin voltage → loop current.
// Industrial Shields 4–20 mA inputs use a ~165 Ω shunt.
#ifndef DIAG_SHUNT_OHMS
#define DIAG_SHUNT_OHMS 165.0f
#endif

// Front-end divider ratio for the 0–10 V interpretation
// (field volts = pin volts × ratio). ~3.03 maps 10 V → ~3.3 V at the ADC.
#ifndef DIAG_V_DIVIDER
#define DIAG_V_DIVIDER 3.0303f
#endif

// Cap the CSV file so runaway logging can't exhaust flash (bytes).
#ifndef DIAG_MAX_BYTES
#define DIAG_MAX_BYTES (512UL * 1024UL)
#endif

#if DIAG_ENABLED
// Mount LittleFS, configure probe pins, register HTTP routes.
// Call once, after setupNetwork().
void diagSetup();

// Sample the probe pins on the configured interval and append a CSV row.
// Call every iteration of loop().
void diagLoop();
#else
static inline void diagSetup() {}
static inline void diagLoop() {}
#endif
