#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

#define N 5

// ================= GPIO (OK ESP32) =================
// INPUT ONLY pins: 34,35,36,39 (OK come COLS solo input)
// OUTPUT pins: 32,33,25,26,27 (ROWS)
static const uint8_t rows[N] = {32, 33, 25, 26, 27};
static const uint8_t cols[N] = {34, 35, 12, 13, 14}; // 14 OK output-safe fallback

// ================= DATA =================
static float raw[N][N];
static float smooth[N][N];
static float baseline[N][N];

// scan state
static uint8_t r = 0;
static uint8_t c = 0;

// smoothing
static const float alpha = 0.18f;

// timing control
static uint32_t lastScan = 0;
static const uint32_t SCAN_INTERVAL_US = 800; // lento e stabile

// ================= GPIO SAFE CONTROL =================

static inline void setInput(uint8_t pin) {
    pinMode(pin, INPUT);
}

static inline void setOutputLow(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

static inline void setOutputHigh(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

// ================= RC MEASURE (SAFE) =================

uint32_t measureRC(uint8_t rr, uint8_t cc) {

    uint8_t col = cols[cc];
    uint8_t row = rows[rr];

    // discharge
    setOutputLow(col);
    delayMicroseconds(30);

    setInput(col);
    setOutputHigh(row);

    uint32_t start = micros();
    uint32_t timeout = start + 3000;

    while (digitalRead(col) == LOW) {
        if ((int32_t)(micros() - timeout) > 0) break;
        yield(); // IMPORTANTISSIMO anti watchdog
    }

    uint32_t dt = micros() - start;

    setInput(row);

    return dt;
}

// normalize
static inline float norm(uint32_t v) {
    if (v > 2000) v = 2000;
    return (float)v / 2000.0f;
}

// ================= SCAN =================

void heatmapLoop() {

    if (micros() - lastScan < SCAN_INTERVAL_US) return;
    lastScan = micros();

    uint32_t t = measureRC(r, c);
    float v = norm(t);

    raw[r][c] = v;

    // EMA smoothing
    smooth[r][c] = smooth[r][c] + alpha * (v - smooth[r][c]);

    // baseline slow adaptation
    baseline[r][c] = baseline[r][c] * 0.999f + v * 0.001f;

    // next cell
    c++;
    if (c >= N) {
        c = 0;
        r++;
        if (r >= N) r = 0;
    }
}

// ================= HTTP =================

void heatmapHandle(AsyncWebServerRequest *request) {

    StaticJsonDocument<1024> doc;

    JsonArray arr = doc.createNestedArray("raw");
    JsonArray arr2 = doc.createNestedArray("smooth");

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            arr.add(raw[i][j]);
            arr2.add(smooth[i][j]);
        }
    }

    String out;
    serializeJson(doc, out);

    request->send(200, "application/json", out);
}

// reset calibration
void calibrateHandle(AsyncWebServerRequest *request) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            baseline[i][j] = raw[i][j];
            smooth[i][j] = 0;
        }
    }
    request->send(200, "text/plain", "OK");
}

// ================= INIT =================

void heatmapInit() {

    for (int i = 0; i < N; i++) {
        pinMode(rows[i], INPUT);
        pinMode(cols[i], INPUT);
    }

    server.on("/heatmap", HTTP_GET, heatmapHandle);
    server.on("/calibrate", HTTP_GET, calibrateHandle);
}