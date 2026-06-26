// OTA Configuration - UPDATE THESE VALUES FOR YOUR PROJECT
const OTA_CONFIG = {
    // GitHub repository URL for firmware updates (raw.githubusercontent.com format)
    // Format: https://raw.githubusercontent.com/OWNER/REPO/firmware_prod
    GITHUB_BASE_URL: "https://github.com/matteopedani/cuscino-esp32.git",

    // Endpoints (usually don't need to change)
    VERSION_JSON: "/version.json",
    FIRMWARE_BIN: "/build/firmware.bin",
    SPIFFS_BIN: "/build/spiffs.bin"
};
