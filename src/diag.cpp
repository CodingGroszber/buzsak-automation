#include "diag.h"

#if DIAG_ENABLED

#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>
#include "wifi_network.h" // extern WebServer server;

// Uncalibrated ADC read from the framework — bypasses the 4–20 mA
// calibration/sentinel that the public analogRead() applies on 32/33.
extern "C" uint16_t __analogRead(uint32_t pin);

namespace
{
    constexpr char CSV_PATH[] = "/diag.csv";
    constexpr char CSV_HEADER[] =
        "uptime_ms,label,gpio,adc_raw,adc_cal,millivolts,current_mA,voltage_V\n";

    struct ProbePin
    {
        const char *label; // human tag incl. assumed mode
        int gpio;          // ESP32 GPIO
        bool currentMode;  // true = interpret as 4–20 mA, false = 0–10 V
    };

    // All four candidate analog pins across both board variants.
    const ProbePin PROBES[] = {
        {"I0.7_MA", 32, true}, // 4–20 mA channel 0 (this board)
        {"I0.8_MA", 33, true}, // 4–20 mA channel 1 (this board)
        {"I0.7_V", 34, false}, // 0–10 V channel 0 (other variant)
        {"I0.8_V", 35, false}, // 0–10 V channel 1 (other variant)
    };
    constexpr int NUM_PROBES = sizeof(PROBES) / sizeof(PROBES[0]);

    unsigned long lastSampleMs = 0;
    bool fsReady = false;
    bool capReached = false;

    void ensureHeader()
    {
        if (!fsReady)
            return;
        if (!LittleFS.exists(CSV_PATH))
        {
            File f = LittleFS.open(CSV_PATH, FILE_WRITE);
            if (f)
            {
                f.print(CSV_HEADER);
                f.close();
            }
        }
    }

    // Build one CSV row for a probe pin from a fresh set of readings.
    String rowFor(const ProbePin &p, unsigned long t)
    {
        uint16_t adcRaw = __analogRead(p.gpio);
        int adcCal = analogRead(p.gpio);
        uint32_t mv = analogReadMilliVolts(p.gpio);

        // Both interpretations are always logged; only the relevant one is
        // physically meaningful for a given wiring, the other is a sanity check.
        float current_mA = (mv / 1000.0f) / DIAG_SHUNT_OHMS * 1000.0f;
        float voltage_V = (mv / 1000.0f) * DIAG_V_DIVIDER;

        String row = String(t) + "," + p.label + "," + String(p.gpio) + ",";
        row += String(adcRaw) + "," + String(adcCal) + "," + String(mv) + ",";
        row += String(current_mA, 3) + "," + String(voltage_V, 3) + "\n";
        return row;
    }

    void appendSample()
    {
        unsigned long t = millis();
        String block;
        for (int i = 0; i < NUM_PROBES; i++)
            block += rowFor(PROBES[i], t);

        Serial.print(block); // live CSV to serial as well

        if (!fsReady || capReached)
            return;

        File f = LittleFS.open(CSV_PATH, FILE_APPEND);
        if (!f)
            return;
        if (f.size() + block.length() > DIAG_MAX_BYTES)
        {
            capReached = true;
            Serial.println("[DIAG] CSV size cap reached — logging paused.");
        }
        else
        {
            f.print(block);
        }
        f.close();
    }

    // ── HTTP: download the raw CSV ────────────────────────────
    void handleDiagCsv()
    {
        if (!fsReady || !LittleFS.exists(CSV_PATH))
        {
            server.send(404, "text/plain", "No diagnostic log yet.");
            return;
        }
        File f = LittleFS.open(CSV_PATH, FILE_READ);
        server.sendHeader("Content-Disposition", "attachment; filename=diag.csv");
        server.streamFile(f, "text/csv");
        f.close();
    }

    // ── HTTP: clear the log ───────────────────────────────────
    void handleDiagClear()
    {
        if (fsReady && LittleFS.exists(CSV_PATH))
            LittleFS.remove(CSV_PATH);
        capReached = false;
        ensureHeader();
        server.send(200, "text/plain", "Diagnostic log cleared.");
    }

    // ── HTTP: live readings page ──────────────────────────────
    void handleDiagPage()
    {
        unsigned long t = millis();
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
        html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<meta http-equiv='refresh' content='2'><title>Analog Probe</title>";
        html += "<style>body{font-family:monospace;background:#111;color:#0f0;padding:20px}";
        html += "table{border-collapse:collapse;width:100%;max-width:760px}";
        html += "th,td{border:1px solid #333;padding:6px 10px;text-align:right}";
        html += "th{color:#0ff}td:first-child,th:first-child{text-align:left}";
        html += "a{color:#0ff}</style></head><body>";
        html += "<h2>Analog Probe Diagnostic</h2>";
        html += "<p>shunt=" + String(DIAG_SHUNT_OHMS, 1) + "&#8486; &nbsp; divider=" + String(DIAG_V_DIVIDER, 4) + "</p>";
        html += "<table><tr><th>Pin</th><th>GPIO</th><th>adc_raw</th><th>adc_cal</th>";
        html += "<th>mV</th><th>current_mA</th><th>voltage_V</th></tr>";
        for (int i = 0; i < NUM_PROBES; i++)
        {
            const ProbePin &p = PROBES[i];
            uint16_t adcRaw = __analogRead(p.gpio);
            int adcCal = analogRead(p.gpio);
            uint32_t mv = analogReadMilliVolts(p.gpio);
            float current_mA = (mv / 1000.0f) / DIAG_SHUNT_OHMS * 1000.0f;
            float voltage_V = (mv / 1000.0f) * DIAG_V_DIVIDER;
            html += "<tr><td>" + String(p.label) + "</td><td>" + String(p.gpio) + "</td>";
            html += "<td>" + String(adcRaw) + "</td><td>" + String(adcCal) + "</td>";
            html += "<td>" + String(mv) + "</td><td>" + String(current_mA, 2) + "</td>";
            html += "<td>" + String(voltage_V, 3) + "</td></tr>";
        }
        html += "</table><p>uptime " + String(t / 1000) + " s";
        html += (capReached ? " &nbsp; <b>[log paused: size cap]</b>" : "") + String("</p>");
        html += "<p><a href='/diag.csv'>download CSV</a> &nbsp; | &nbsp; ";
        html += "<a href='/diag/clear'>clear log</a></p></body></html>";
        server.send(200, "text/html", html);
    }
}

void diagSetup()
{
    for (int i = 0; i < NUM_PROBES; i++)
        pinMode(PROBES[i].gpio, INPUT);

    fsReady = LittleFS.begin(true); // format on first use if needed
    if (fsReady)
        ensureHeader();
    else
        Serial.println("[DIAG] LittleFS mount failed — CSV logging disabled.");

    server.on("/diag", HTTP_GET, handleDiagPage);
    server.on("/diag.csv", HTTP_GET, handleDiagCsv);
    server.on("/diag/clear", HTTP_GET, handleDiagClear);

    Serial.println("[DIAG] Analog probe active — http://" + WiFi.localIP().toString() + "/diag");
    Serial.print(CSV_HEADER);
}

void diagLoop()
{
    unsigned long now = millis();
    if (now - lastSampleMs < DIAG_INTERVAL_MS)
        return;
    lastSampleMs = now;
    appendSample();
}

#endif // DIAG_ENABLED
