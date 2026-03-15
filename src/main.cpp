#include <Wire.h>
#include <WiFi.h>
#include <time.h>

#include "ConfigurationManager.h"
#include "BluetoothManager.h"
#include "ApplicationManager.h"
#include "PrinterManager.h"
#include "NFCManager.h"
#include "PrusaLinkAPIStrategy.h"
#include "SpoolmanManager.h"
#include "HomeAssistantManager.h"
#include "StubPrinterLinkStrategy.h"
#if USE_LCD
#include "LCDManager.h"
#endif


#if USE_STATUS_LED
#include "LEDManager.h"
LEDManager ledManager;
#endif

// Global HTTP mutex for serializing WiFi HTTP requests
SemaphoreHandle_t g_httpMutex = nullptr;

#if USE_LCD
// LCD I2C pins
#define LCD_SDA 23
#define LCD_SCL 22

// LCD Manager
LCDManager lcdManager(0x27, 16, 2);
#endif

void initWiFi() {
  auto& config = ConfigurationManager::getInstance();

  if (strlen(config.getWiFiSSID()) == 0) {
    Serial.println("WiFi SSID not configured - skipping WiFi");
#if USE_LCD
    lcdManager.updateScreen("WiFi: no SSID", "Configure via BLE");
#endif
    delay(2000);
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(config.getWiFiSSID());

#if USE_LCD
  lcdManager.updateScreen("Connecting WiFi", "");
#endif

  WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());

#if USE_LCD
    lcdManager.updateScreen("WiFi OK", WiFi.localIP().toString().c_str());
#endif

#if USE_STATUS_LED
    ledManager.showWifiConnected();  // network up — not yet fully initialized
#endif

    Serial.println("Setting up NTP...");
    configTime(0, 0, "pool.ntp.org");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
#if USE_LCD
      lcdManager.updateScreen("NTP FAILED", "");
#endif
    } else {
      Serial.println("Time obtained");
    }

    delay(2000);
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");

#if USE_LCD
    lcdManager.updateScreen("WiFi FAILED", "");
#endif

#if USE_STATUS_LED
    ledManager.showWifiFailed();
#endif

    delay(2000);
  }
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  delay(1000);
  Serial.println("=== Starting setup ===");

#if USE_STATUS_LED
  ledManager.begin(STATUS_LED_PIN);
  ledManager.showBooting();
#endif

#if USE_LCD
  // Initialize I2C with custom pins for LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  Serial.println("I2C initialized");

  // Initialize LCD and start its task on core 0
  lcdManager.begin();
  lcdManager.startTask();
  lcdManager.updateScreen("Initializing...", "");
  Serial.println("LCD initialized");
#endif

  // Initialize ConfigurationManager FIRST (loads NVS)
  if (!ConfigurationManager::getInstance().begin()) {
    Serial.println("ConfigurationManager init failed - halting");
#if USE_LCD
    lcdManager.updateScreen("Config FAILED", "");
#endif
    while (1) { delay(1000); }
  }
#if USE_LCD
  lcdManager.setScreenTimeoutMs(ConfigurationManager::getInstance().getLcdTimeoutMs());
#endif

  // Initialize ApplicationManager (message queue) with optional LCD reference
#if USE_LCD
  if (!ApplicationManager::getInstance().begin(&lcdManager)) {
#else
  if (!ApplicationManager::getInstance().begin(nullptr)) {
#endif
    Serial.println("ApplicationManager init failed - halting");
#if USE_LCD
    lcdManager.updateScreen("AppMgr FAILED", "");
#endif
    while (1) { delay(1000); }
  }

  // Initialize BluetoothManager BEFORE WiFi (they share the radio)
#if USE_LCD
  lcdManager.updateScreen("Starting BLE...", "");
#endif
  if (!BluetoothManager::getInstance().begin()) {
    Serial.println("BluetoothManager init failed - continuing without BLE");
  }

  // Connect to WiFi
  initWiFi();

  // Create global HTTP mutex for serializing HTTP requests
  g_httpMutex = xSemaphoreCreateMutex();
  if (g_httpMutex == nullptr) {
    Serial.println("Failed to create HTTP mutex - halting");
#if USE_LCD
    lcdManager.updateScreen("Mutex FAILED", "");
#endif
    while (1) { delay(1000); }
  }

  // Initialize SpoolmanManager
  if (!SpoolmanManager::getInstance().begin(g_httpMutex)) {
    Serial.println("SpoolmanManager init failed - continuing without Spoolman");
  }

  // Initialize HomeAssistantManager
  if (!HomeAssistantManager::getInstance().begin()) {
    Serial.println("HomeAssistantManager init failed - continuing without HA");
  }

  // Load automation mode from config
  {
    uint8_t mode = ConfigurationManager::getInstance().getAutomationMode();
    ApplicationManager::getInstance().setAutomationMode(static_cast<AutomationMode>(mode));
    Serial.printf("Automation mode: %s\n",
                  mode == 0 ? "SELF_DIRECTED" : "CONTROLLED_BY_HA");
  }

  // Initialize NFCManager
  if (!NFCManager::getInstance().begin()) {
    Serial.println("NFCManager init failed - halting");
#if USE_LCD
    lcdManager.updateScreen("NFC FAILED", "");
#endif
    while (1) { delay(1000); }
  }

  // Initialize and start PrinterManager
  static PrusaLinkAPIStrategy printerStrategy;
  printerStrategy.setHttpMutex(g_httpMutex);
  PrinterManager::getInstance().setStrategy(&printerStrategy);
  PrinterManager::getInstance().begin();

  // One synchronous check before polling task starts (no race condition)
  printerStrategy.update();

  PrinterManager::getInstance().startPollingTask();

  // Start NFC scan task
  NFCManager::getInstance().startScanTask();

  // Start SpoolmanManager task
  SpoolmanManager::getInstance().startTask();

  // Start HomeAssistantManager task
  auto& config = ConfigurationManager::getInstance();
  Serial.printf("Setup: HA config before startTask: enabled=%s host='%s' host_len=%u port=%u user_set=%s\n",
                config.getHAEnabled() ? "true" : "false",
                config.getHAMqttHost(),
                static_cast<unsigned>(strlen(config.getHAMqttHost())),
                static_cast<unsigned>(config.getHAMqttPort()),
                strlen(config.getHAMqttUser()) > 0 ? "true" : "false");
  HomeAssistantManager::getInstance().startTask();

#if USE_LCD
  ApplicationManager::getInstance().showStatusOnLCD();
#endif

#if USE_STATUS_LED
  ledManager.showReady();  // NFC + Spoolman + HA + scanner all initialized
#endif

  Serial.println("=== Setup complete ===");
}

void loop() {
  // Process any pending messages for the application
  ApplicationManager::getInstance().processMessages();

  // LCD and NFC scanning are handled by their own tasks
  delay(100);
}