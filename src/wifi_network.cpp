#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ElegantOTA.h>
#include "io_config.h"
#include "logic.h"
#include "wifi_network.h"
#include "html_assets.h"

// ── WiFi credentials ─────────────────────────────────────────
const char *ssid = "neunet";
const char *password = "20210228";

// ── Web server on port 80 ────────────────────────────────────
WebServer server(80);

// ── WiFi connect ─────────────────────────────────────────────
void connectWiFi()
{
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int led = wifiLedIndex();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (led >= 0)
        {
            digitalWrite(OUTPUTS[led].pin, HIGH);
            delay(250);
            digitalWrite(OUTPUTS[led].pin, LOW);
            delay(250);
        }
        else
            delay(500);
        Serial.print(".");
    }
    if (led >= 0)
        digitalWrite(OUTPUTS[led].pin, HIGH); // solid ON = connected
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

// ── GET /api/state ────────────────────────────────────────────
// Returns full IO state as JSON — consumed by app and Flask server
static void handleApiState()
{
    String json = "{";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"switch_mode\":" + String(plcSwitchMode()) + ",";
    json += "\"refill_button\":" + String(refillButtonIsHigh() ? "true" : "false") + ",";
    json += "\"refill_active\":" + String(refillIsActive() ? "true" : "false") + ",";

    json += "\"outputs\":[";
    for (int i = 0; i < NUM_OUTPUTS; i++)
    {
        if (i)
            json += ",";
        bool s = digitalRead(OUTPUTS[i].pin);
        json += "{\"name\":\"" + String(OUTPUTS[i].name) + "\",";
        json += "\"label\":\"" + String(OUTPUTS[i].label) + "\",";
        json += "\"state\":" + String(s ? "true" : "false") + ",";
        json += "\"controllable\":" + String(OUTPUTS[i].controllable ? "true" : "false") + "}";
    }
    json += "],";

    json += "\"analog\":[";
    for (int i = 0; i < NUM_ANALOG_SENSORS; i++)
    {
        if (i)
            json += ",";
        uint32_t mv = analogReadMilliVolts(ANALOG_INPUTS[i].pin);
        String analogName = String(ANALOG_INPUTS[i].name);
        bool isPressure = (analogName == "pressure");
        bool isWaterLevel = (analogName == "water_level");
        bool valid = isPressure ? (mv > PRESSURE_MIN_VALID_MV) : (mv > 0);
        json += "{\"name\":\"" + String(ANALOG_INPUTS[i].name) + "\",";
        json += "\"label\":\"" + String(ANALOG_INPUTS[i].label) + "\",";
        json += "\"mv\":" + String(mv) + ",";
        json += "\"signal\":" + String(valid ? "true" : "false");
        if (valid && isPressure)
            json += ",\"bar\":" + String(pressureMvToBar(mv), 2);
        if (isWaterLevel)
        {
            float liters = waterLevelMvToLiters(mv);
            json += ",\"liters\":" + String(liters, 0);
            json += ",\"low\":" + String(liters <= WATER_LEVEL_LOW_L ? "true" : "false");
        }
        json += "}";
    }
    json += "]";

    json += "}";
    server.send(200, "application/json", json);
}

// ── POST /api/control ─────────────────────────────────────────
// Form body: name=refill&state=<1|0>
static void handleApiControl()
{
    String name = server.arg("name");
    String stateStr = server.arg("state");

    if (name.isEmpty() || stateStr.isEmpty())
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing name or state\"}");
        return;
    }

    if (name != "refill")
    {
        server.send(403, "application/json", "{\"ok\":false,\"error\":\"Only refill is controllable\"}");
        return;
    }

    bool newState;
    if (stateStr == "1" || stateStr == "true")
        newState = true;
    else if (stateStr == "0" || stateStr == "false")
        newState = false;
    else
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"State must be true or false\"}");
        return;
    }

    if (!newState)
    {
        String buttonState = refillButtonIsHigh() ? "true" : "false";
        server.send(200, "application/json", "{\"ok\":true,\"refill_button\":" + buttonState + "}");
        return;
    }

    if (plcSwitchMode() != 1)
    {
        server.send(409, "application/json", "{\"ok\":false,\"error\":\"Selector must be in automatic mode\"}");
        return;
    }
    if (refillIsActive() || refillButtonIsHigh())
    {
        server.send(409, "application/json", "{\"ok\":false,\"error\":\"Refill already requested or active\"}");
        return;
    }

    TriggerRefill(true);
    server.send(200, "application/json", "{\"ok\":true,\"refill_button\":true}");
    Serial.println("[CTRL] refill requested");
}

// ── GET / — serve the HTML dashboard from Flash ──────────────
static void handleRoot()
{
    server.send_P(200, "text/html", INDEX_HTML);
}

// ── Network setup (called once from main setup()) ─────────────
void setupNetwork()
{
    connectWiFi();

    // ArduinoOTA
    ArduinoOTA.setHostname("PLC-14IO-Cabinet1");
    ArduinoOTA.setPassword("askim");
    ArduinoOTA.onStart([]()
                       {
    int led = wifiLedIndex();
    if (led >= 0)
    {
      for (int i = 0; i < 10; i++)
      {
        digitalWrite(OUTPUTS[led].pin, HIGH); delay(100);
        digitalWrite(OUTPUTS[led].pin, LOW);  delay(100);
      }
    }
    // Safety: turn all controllable outputs OFF during flash
    for (int i = 0; i < NUM_OUTPUTS; i++)
      if (OUTPUTS[i].controllable) digitalWrite(OUTPUTS[i].pin, LOW);
    Serial.println("OTA starting — outputs forced OFF."); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("OTA complete! Rebooting..."); });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t)
                          { Serial.printf("OTA: %u%%\n", p / (t / 100)); });
    ArduinoOTA.onError([](ota_error_t e)
                       { Serial.printf("OTA Error [%u]\n", e); });
    ArduinoOTA.begin();

    // HTTP routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/state", HTTP_GET, handleApiState);
    server.on("/api/control", HTTP_POST, handleApiControl);

    ElegantOTA.setAuth("neugarden", "askim");
    ElegantOTA.begin(&server);
    server.begin();

    Serial.println(">>> Firmware " + String(FIRMWARE_VERSION) + " — boot OK");
    Serial.println("  Status : http://" + WiFi.localIP().toString() + "/");
    Serial.println("  State  : http://" + WiFi.localIP().toString() + "/api/state");
    Serial.println("  Control: POST http://" + WiFi.localIP().toString() + "/api/control");
}

// ── Network loop (called every iteration of main loop()) ──────
void loopNetwork()
{
    ArduinoOTA.handle();
    server.handleClient();
    ElegantOTA.loop();

    // WiFi watchdog
    if (WiFi.status() != WL_CONNECTED)
    {
        int led = wifiLedIndex();
        if (led >= 0)
            digitalWrite(OUTPUTS[led].pin, LOW);
        Serial.println("WiFi lost! Reconnecting...");
        connectWiFi();
    }
}
