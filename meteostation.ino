#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SdsDustSensor.h>

const char * wifi_ssid = "SpiderNet";
const char * wifi_password = "81880728550737648282";

/* SDS011 Dust Sensor */
const int SDS_RX_PIN = D2;
const int SDS_TX_PIN = D1;
SoftwareSerial softwareSerial(SDS_RX_PIN, SDS_TX_PIN);
SdsDustSensor sds(softwareSerial); //  additional parameters: retryDelayMs and maxRetriesNotAvailable
const int MINUTE = 60000;
const int WAKEUP_WORKING_TIME = 30000; // 30 seconds.
const int MEASUREMENT_INTERVAL = 5 * MINUTE;

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(D7, DHTTYPE);
ESP8266WebServer webServer(80);

// Current time
unsigned long currentTime = millis();

// Previous time
unsigned long previousTime = 0;

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

String header;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  setupSDS();
  connectToWiFi();
}

void setupSDS() {
  sds.begin();
  Serial.print("SDS011 ");
  Serial.println(sds.queryFirmwareVersion().toString());
  // Ensures SDS011 is in 'query' reporting mode:
  Serial.println(sds.setQueryReportingMode().toString());
}

void connectToWiFi() {
  Serial.printf("Connecting to '%s'\n", wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    webServer.on("/current", handleWebServerRequest);
    webServer.begin();
  } else {
    Serial.println("Connection Failed!");
  }
}

void loop() {
  webServer.handleClient();
}

void handleWebServerRequest() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  readDataFromSDS();

  Serial.println("------------------Somebody requested me");
  String message = "";
  message += "<!DOCTYPE html>";
  message += "<html>";
  message += "<head>";
  message += "<title>Meteostation</title>";
  message += "</head>";
  message += "<body>";
  message += "<h1>Hi there! I am a Munties Meteostation!</h1>";
  message += "<h1>Humidity: " + String(humidity) + "%</h1>";
  message += "<h1>Temperature: " + String(temperature) + " C</h1>";
  message += "</body>";
  message += "</html>";
  
  webServer.send(200, "text/html", message);
}

void readDataFromSDS()
{
  sds.wakeup();
  //delay(WAKEUP_WORKING_TIME);
  delay(100);
  // Get data from SDS011
  PmResult pm = sds.queryPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25); // float, μg/m3
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }
  
  // Put SDS011 back to sleep
  WorkingStateResult state = sds.sleep();
  
  if (state.isWorking()) {
    Serial.println("Problem with sleeping the SDS011 sensor.");
  } else {
    Serial.println("SDS011 sensor is sleeping");
  }
}
