#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SdsDustSensor.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

// Configuration constants
const char* WIFI_SSID = "wifi";
const char* WIFI_PASSWORD = "123456";
const char* INFLUXDB_URL = "http://192.168.179.48:8086";
const char* INFLUXDB_TOKEN = "Jdt0cYftq-WnkbosSGF-vA3KM-d-c1nNCc7z4Qq-EJqupSniEQgGQNnlEgwtesBrv6PCMlyPkVfBHB37Cx0xyQ==";
const char* INFLUXDB_ORG = "3cbd862919cba2c9";
const char* INFLUXDB_BUCKET = "home-sensors";
const char* TZ_INFO = "UTC2";

// Timing constants
const unsigned long DELAY_AFTER_SUCCESS = 5000;      // 5 seconds
const unsigned long DELAY_AFTER_ERROR = 10000;      // 10 seconds
const unsigned long WIFI_RETRY_DELAY = 5000;        // 5 seconds
const unsigned long SENSOR_READ_TIMEOUT = 2000;     // 2 seconds

// Pin definitions
const int SDS_RX_PIN = D1;
const int SDS_TX_PIN = D2;
const int DHT_PIN = D7;

// Sensor instances
SdsDustSensor sds(SDS_RX_PIN, SDS_TX_PIN);
DHT dht(DHT_PIN, DHT22);
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Global variables
String chipId;
bool isWifiConnected = false;
unsigned long lastSensorRead = 0;
unsigned long lastWifiCheck = 0;

// LED blinking variables
bool isBlinking = false;
unsigned long blinkStartTime = 0;
unsigned long lastBlinkToggle = 0;
const unsigned long BLINK_DURATION = 1000;     // 1 second total blink time
const unsigned long BLINK_INTERVAL = 100;      // 100ms on/off intervals (fast blink)

// Sensor reading timing
const unsigned long SENSOR_READ_INTERVAL = 30000; // Read sensors every 30 seconds

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP8266 Sensor Station Starting ===");
    
    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED off initially
    
    // Get chip ID for identification
    chipId = String(ESP.getChipId());
    Serial.println("Chip ID: " + chipId);
    
    // Initialize sensors
    initializeSensors();
    
    // Connect to WiFi
    connectToWiFi();
    
    // Initialize InfluxDB client
    if (isWifiConnected) {
        setupInfluxDB();
    }
    
    Serial.println("=== Setup Complete ===\n");
}

void initializeSensors() {
    Serial.println("Initializing sensors...");
    
    // Initialize DHT sensor
    dht.begin();
    Serial.println("DHT22 sensor initialized");
    
    // Initialize SDS011 sensor
    if (setupSDS()) {
        Serial.println("SDS011 dust sensor initialized successfully");
    } else {
        Serial.println("Warning: SDS011 initialization failed");
    }
}

bool setupSDS() {
    sds.begin();
    
    // Get firmware version
    FirmwareVersionResult version = sds.queryFirmwareVersion();
    if (version.isOk()) {
        Serial.println("SDS011 Firmware: " + version.toString());
    } else {
        Serial.println("Failed to get SDS011 firmware version");
        return false;
    }
    
    // Set active reporting mode
    ReportingModeResult reportingMode = sds.setActiveReportingMode();
    if (!reportingMode.isOk()) {
        Serial.println("Failed to set SDS011 reporting mode");
        return false;
    }
    
    // Set continuous working period - FIXED: Use correct return type
    WorkingPeriodResult workingPeriod = sds.setContinuousWorkingPeriod();
    if (!workingPeriod.isOk()) {
        Serial.println("Failed to set SDS011 working period");
        return false;
    }
    
    return true;
}

void connectToWiFi() {
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Wait for connection with timeout
    int attempts = 0;
    const int maxAttempts = 20; // 10 seconds total
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        isWifiConnected = true;
        Serial.println();
        Serial.println("WiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // === SIGNAL STRENGTH MONITORING ===
        int rssi = WiFi.RSSI();
        Serial.print("Signal strength: ");
        Serial.print(rssi);
        Serial.print(" dBm (");
        if (rssi > -50) Serial.print("Excellent");
        else if (rssi > -60) Serial.print("Good");
        else if (rssi > -70) Serial.print("Fair");
        else Serial.print("Weak");
        Serial.println(")");
        // === END SIGNAL STRENGTH MONITORING ===
        
        updateLEDStatus(); // Update LED to show connection status
    } else {
        isWifiConnected = false;
        Serial.println();
        Serial.println("WiFi connection failed!");
        updateLEDStatus(); // Update LED to show disconnection
    }
}

void setupInfluxDB() {
    Serial.println("Setting up InfluxDB connection...");
    
    // Check server connection
    if (influxClient.validateConnection()) {
        Serial.println("Connected to InfluxDB successfully!");
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influxClient.getLastErrorMessage());
    }
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle LED blinking during data transmission (NON-BLOCKING!)
    handleLEDBlinking(currentTime);
    
    // Check WiFi connection periodically
    if (currentTime - lastWifiCheck > 30000) { // Check every 30 seconds
        checkWiFiConnection();
        lastWifiCheck = currentTime;
    }
    
    // Only proceed if WiFi is connected
    if (!isWifiConnected) {
        delay(1000); // Short delay when WiFi disconnected
        return;
    }
    
    // Read and send sensor data at intervals (NON-BLOCKING!)
    if (currentTime - lastSensorRead > SENSOR_READ_INTERVAL) {
        readAndSendSensorData();
        lastSensorRead = currentTime;
    }
    
    // Small delay to prevent overwhelming the CPU
    delay(100); // 100ms delay - perfect for LED blinking timing!
}

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost. Attempting to reconnect...");
        isWifiConnected = false;
        updateLEDStatus(); // Update LED to show disconnection
        connectToWiFi();
    } else if (!isWifiConnected) {
        // WiFi was reconnected
        isWifiConnected = true;
        updateLEDStatus(); // Update LED to show connection status
        Serial.println("WiFi reconnected!");
        
        // === PERIODIC SIGNAL STRENGTH MONITORING ===
        int rssi = WiFi.RSSI();
        Serial.print("Current signal: ");
        Serial.print(rssi);
        Serial.print(" dBm (");
        if (rssi > -50) Serial.print("Excellent");
        else if (rssi > -60) Serial.print("Good");
        else if (rssi > -70) Serial.print("Fair");
        else Serial.print("Weak");
        Serial.println(")");
        // === END PERIODIC MONITORING ===
    }
}

void readAndSendSensorData() {
    // Read DHT22 sensor
    readAndSendDHTData();
    
    // Small delay between sensor readings
    delay(100);
    
    // Read SDS011 sensor
    readAndSendDustData();
}

void readAndSendDHTData() {
    Serial.println("Reading DHT22 sensor...");
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    // Validate readings
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    
    // Sanity check values
    if (humidity < 0 || humidity > 100 || temperature < -40 || temperature > 80) {
        Serial.println("DHT sensor readings out of range!");
        Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temperature, humidity);
        return;
    }
    
    Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temperature, humidity);
    
    // Send to InfluxDB
    startDataTransmissionBlink(); // Start blinking before sending
    bool success = sendDataToInfluxDB("temperature", temperature, "humidity", humidity);
    if (success) {
        Serial.println("DHT data sent successfully");
    } else {
        Serial.println("Failed to send DHT data");
    }
    
    // Non-blocking delay after transmission
    delay(500); // Short delay between sensor readings
}

void readAndSendDustData() {
    Serial.println("Reading SDS011 dust sensor...");
    
    PmResult pm = sds.readPm();
    
    if (!pm.isOk()) {
        Serial.println("SDS011 read error: " + pm.statusToString());
        delay(DELAY_AFTER_ERROR);
        return;
    }
    
    // Validate readings
    if (pm.pm25 < 0 || pm.pm10 < 0 || pm.pm25 > 1000 || pm.pm10 > 1000) {
        Serial.println("SDS011 readings out of range!");
        Serial.printf("PM2.5: %.2f μg/m³, PM10: %.2f μg/m³\n", pm.pm25, pm.pm10);
        return;
    }
    
    Serial.printf("PM2.5: %.2f μg/m³, PM10: %.2f μg/m³\n", pm.pm25, pm.pm10);
    
    // Send to InfluxDB
    startDataTransmissionBlink(); // Start blinking before sending
    bool success = sendDataToInfluxDB("pm25", pm.pm25, "pm10", pm.pm10);
    if (success) {
        Serial.println("Dust data sent successfully");
    } else {
        Serial.println("Failed to send dust data");
    }
    
    // No additional delay here - let the main loop handle timing
}

bool sendDataToInfluxDB(const String& field1, float value1, const String& field2, float value2) {
    if (!isWifiConnected) {
        Serial.println("Cannot send data: WiFi not connected");
        return false;
    }
    
    // Create data point
    Point sensorData("sensor_data");
    sensorData.addTag("device", "esp8266_" + chipId);
    sensorData.addTag("location", "home"); // Add location tag for better organization
    sensorData.addField(field1, value1);
    sensorData.addField(field2, value2);
    
    // Attempt to write data
    if (influxClient.writePoint(sensorData)) {
        Serial.println("✓ Data written to InfluxDB successfully");
        return true;
    } else {
        Serial.print("✗ InfluxDB write failed: ");
        Serial.println(influxClient.getLastErrorMessage());
        
        // Try to reconnect if connection was lost
        if (!influxClient.validateConnection()) {
            Serial.println("Attempting to reconnect to InfluxDB...");
            setupInfluxDB();
        }
        
        return false;
    }
}

// ===== LED BLINKING FUNCTIONS =====
void startDataTransmissionBlink() {
    Serial.println("📡 Starting data transmission - LED blinking for 1 second");
    isBlinking = true;
    blinkStartTime = millis();
    lastBlinkToggle = blinkStartTime;
    digitalWrite(LED_BUILTIN, LOW); // Start with LED on
}

void handleLEDBlinking(unsigned long currentTime) {
    if (!isBlinking) return;
    
    // Check if blink duration has elapsed
    if (currentTime - blinkStartTime >= BLINK_DURATION) {
        isBlinking = false;
        updateLEDStatus(); // Return to normal WiFi status indication
        Serial.println("📡 Data transmission blink complete");
        return;
    }
    
    // Toggle LED at specified interval
    if (currentTime - lastBlinkToggle >= BLINK_INTERVAL) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlinkToggle = currentTime;
    }
}

void updateLEDStatus() {
    if (!isBlinking) {
        // Set LED based on WiFi connection status
        digitalWrite(LED_BUILTIN, isWifiConnected ? LOW : HIGH);
    }
}
