// Example main.cpp showing how to integrate OTA updates
// Copy and adapt this to your project

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "ota_updater.h"
#include "version.h"

// WiFi credentials - update these for your network
const char* WIFI_SSID = "alberata";
const char* WIFI_PASS = "grottaferrata";

AsyncWebServer server(80);

String getSpiffsVersion() {
  File file = SPIFFS.open("/version.txt", "r");
  if (!file) return "0.0";
  String version = file.readStringUntil('\n');
  file.close();
  version.trim();
  return version;
}

void handleGetConfig(AsyncWebServerRequest *request) {
  String jsonResponse = "{\"version_firmware\":\"" + String(VERSION_FIRMWARE) + "\""
                        ",\"version_spiffs\":\"" + getSpiffsVersion() + "\"}";
  request->send(200, "application/json", jsonResponse);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting ESP32 OTA Template");

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Mount SPIFFS
  if (!SPIFFS.begin(false)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Required endpoints for OTA
  server.on("/update_firmware", HTTP_GET, handleFirmwareUpdate);
  server.on("/get_config", HTTP_GET, handleGetConfig);

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/");

  server.begin();
  Serial.println("HTTP server started");

  // Initialize OTA endpoint
  setupOTA(server);
  Serial.println("OTA setup complete");

  Serial.printf("Firmware: v%s, SPIFFS: v%s\n", VERSION_FIRMWARE, getSpiffsVersion().c_str());
}

void loop() {
  // Handle scheduled reboot after OTA update
  if (shouldReboot && millis() >= rebootTime) {
    Serial.println("Rebooting now...");
    ESP.restart();
  }

  // Your application code here
}
