#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ElegantOTA.h>
#include "io_config.h"
#include "wifi_network.h"

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

    json += "\"inputs\":[";
    for (int i = 0; i < NUM_INPUTS; i++)
    {
        if (i)
            json += ",";
        bool s = digitalRead(INPUTS[i].pin);
        json += "{\"name\":\"" + String(INPUTS[i].name) + "\",";
        json += "\"label\":\"" + String(INPUTS[i].label) + "\",";
        json += "\"state\":" + String(s ? "true" : "false") + "}";
    }
    json += "],";

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
    json += "]";

    json += "}";
    server.send(200, "application/json", json);
}

// ── POST /api/control ─────────────────────────────────────────
// Form body: name=<output_name>&state=<1|0>
// Only sets the output if controllable=true
static void handleApiControl()
{
    String name = server.arg("name");
    String stateStr = server.arg("state");

    if (name.isEmpty() || stateStr.isEmpty())
    {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing name or state\"}");
        return;
    }

    bool newState = (stateStr == "1" || stateStr == "true");

    for (int i = 0; i < NUM_OUTPUTS; i++)
    {
        if (name.equals(OUTPUTS[i].name))
        {
            if (!OUTPUTS[i].controllable)
            {
                server.send(403, "application/json", "{\"ok\":false,\"error\":\"Not controllable\"}");
                return;
            }
            digitalWrite(OUTPUTS[i].pin, newState ? HIGH : LOW);
            server.send(200, "application/json", "{\"ok\":true}");
            Serial.println("[CTRL] " + name + " -> " + String(newState ? "ON" : "OFF"));
            return;
        }
    }
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"Output not found\"}");
}

// ── GET / — built-in HTML status page ────────────────────────
static void handleRoot()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='2'>";
    html += "<title>PLC Cabinet 1</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;text-align:center;padding:40px}";
    html += "h1{color:#00d4ff}h2{color:#aaa;font-size:.85em;margin:24px 0 8px;text-transform:uppercase;letter-spacing:1px}";
    html += ".card{background:#16213e;border-radius:12px;padding:18px;margin:10px auto;max-width:400px}";
    html += ".label{color:#aaa;font-size:.9em}.value{font-size:1.4em;font-weight:bold;margin-top:4px}";
    html += ".on{color:#00ff99}.off{color:#ff4444}.neutral{color:#00d4ff}";
    html += "a.btn{display:inline-block;margin-top:30px;padding:12px 28px;background:#00d4ff;color:#000;border-radius:8px;text-decoration:none;font-weight:bold}";
    html += "</style></head><body>";
    html += "<h1>&#127981; PLC Cabinet 1</h1>";

    html += "<div class='card'><div class='label'>Firmware</div><div class='value neutral'>" + String(FIRMWARE_VERSION) + "</div></div>";
    html += "<div class='card'><div class='label'>IP Address</div><div class='value neutral'>" + WiFi.localIP().toString() + "</div></div>";

    html += "<h2>Inputs</h2>";
    for (int i = 0; i < NUM_INPUTS; i++)
    {
        bool s = digitalRead(INPUTS[i].pin);
        html += "<div class='card'><div class='label'>" + String(INPUTS[i].label) + "</div>";
        html += "<div class='value " + String(s ? "on" : "off") + "'>" + String(s ? "&#9679; ACTIVE" : "&#9675; INACTIVE") + "</div></div>";
    }

    html += "<h2>Outputs</h2>";
    for (int i = 0; i < NUM_OUTPUTS; i++)
    {
        bool s = digitalRead(OUTPUTS[i].pin);
        html += "<div class='card'><div class='label'>" + String(OUTPUTS[i].label) + "</div>";
        html += "<div class='value " + String(s ? "on" : "off") + "'>" + String(s ? "&#9889; ON" : "&#9675; OFF") + "</div></div>";
    }

    html += "<a class='btn' href='/update'>&#128295; Upload Firmware</a>";
    html += "<p style='color:#555;font-size:.8em;margin-top:20px'>&#128257; Auto-refreshing every 2 seconds</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ── Network setup (called once from main setup()) ─────────────
void setupNetwork()
{
    connectWiFi();

    // ArduinoOTA
    ArduinoOTA.setHostname("PLC-14IO-Cabinet1");
    ArduinoOTA.setPassword("admin");
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
