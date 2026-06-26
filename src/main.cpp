// Example main.cpp showing how to integrate OTA updates
// Copy and adapt this to your project

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "ota_updater.h"
#include "version.h"
#include "heatmap.h"

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


void printBootDebug() {
  Serial.println("\n================ BOOT DEBUG ================");

  // ===== WIFI INFO =====
  Serial.print("WiFi mode: ");
  Serial.println(WiFi.getMode() == WIFI_AP_STA ? "AP+STA" :
                 WiFi.getMode() == WIFI_STA ? "STA" : "AP");

  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("STA connected: ");
  Serial.println(WiFi.isConnected() ? "YES" : "NO");

  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  // ===== GPIO MATRIX =====
  const int rows[5] = {32, 33, 25, 26, 27};
  const int cols[5] = {34, 35, 36, 39, 13};

  Serial.println("\n--- GPIO MATRIX ---");

  Serial.print("ROWS: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(rows[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("COLS: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(cols[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.println("===========================================\n");
}


void setup() {
  Serial.begin(115200);

  // >>> inizializza heatmap
  heatmapInit();


  Serial.println("\nStarting ESP32 cuscino");

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
  printBootDebug();
  heatmapInit();
}


void loop() {
    heatmapLoop();
    delay(2); // fondamentale per WDT + WiFi + OTA
}



