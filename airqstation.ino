#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SdsDustSensor.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

const char * host = "192.168.179.59";
const char * url = "/sensor";
const int httpPort = 5000;
const int delayAfterSuccessfulSendingMs = 5 * 1000;

#define INFLUXDB_URL "http://192.168.179.48:8086"
#define INFLUXDB_TOKEN "Jdt0cYftq-WnkbosSGF-vA3KM-d-c1nNCc7z4Qq-EJqupSniEQgGQNnlEgwtesBrv6PCMlyPkVfBHB37Cx0xyQ=="
#define INFLUXDB_ORG "3cbd862919cba2c9"
#define INFLUXDB_BUCKET "home-sensors"

// Time zone info
#define TZ_INFO "UTC2"

InfluxDBClient influxDbClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

/* WiFi settings */
const char * wifi_ssid = "SpiderNet";
const char * wifi_password = "81880728550737648282";
WiFiClient client;

/* SDS011 Dust Sensor */
const int SDS_RX_PIN = D1;
const int SDS_TX_PIN = D2;
SdsDustSensor sds(SDS_RX_PIN, SDS_TX_PIN);

/* DHT22 Temperature and Humidity Sensor */
#define DHTTYPE    DHT22
DHT dht(D7, DHTTYPE);

String chipId;
int wifiConnectionStatus;

void setup()
{
    // put your setup code here, to run once:
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    dht.begin();
    setupSDS();
    wifiConnectionStatus = connectToWiFi();
    chipId = String(ESP.getChipId());
    Serial.println("chipId=" + chipId);
}

void setupSDS()
{
    sds.begin();
    Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
    Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
    Serial.println(sds.setContinuousWorkingPeriod().toString()); // ensures sensor has continuous working period - default but not recommended
}

int connectToWiFi()
{
    Serial.printf("Connecting to '%s'\n", wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
        Serial.print("Connected. IP: ");
        Serial.println(WiFi.localIP());
        return 1;
    }
    else
    {
        Serial.println("Connection Failed!");
        return 0;
    }
}

void loop()
{
    if (wifiConnectionStatus == 0)
    {
        Serial.println("WiFi is not connected. Try to connect...");
        wifiConnectionStatus = connectToWiFi();
        if (wifiConnectionStatus == 0)
        {
            Serial.println("Unsuccessful attempt");
            delay(5000);
        }
    }
    else
    {
        // Serial.println("WiFi connected");
    }
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (sendDataToInfluxDB("humidity", humidity, "temperature", temperature))
    {
        delay(delayAfterSuccessfulSendingMs);
    }

    PmResult pm = sds.readPm();
    if (pm.isOk())
    {
        if (sendDataToInfluxDB("pm10", pm.pm10, "pm25", pm.pm25))
        {
            delay(delayAfterSuccessfulSendingMs);
        }
    }
    else
    {
        Serial.println("SDS status: " + pm.statusToString());
        delay(10000);
    }
}

int sendDataToInfluxDB(String header1, float val1, String header2, float val2) {
    Point sensorData("sensor_data");
    sensorData.addTag("device", "esp8266_01");
    sensorData.addField(header1, val1);   // replace with actual sensor readings
    sensorData.addField(header2, val2);

    if (influxDbClient.writePoint(sensorData)) {
      Serial.println("Data written successfully.");
      return 1;
    } else {
      Serial.print("Write failed: ");
      Serial.println(influxDbClient.getLastErrorMessage());
      return 0;
    }
}
