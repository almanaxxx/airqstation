#include <DHT.h>
#include <ESP8266WiFi.h>
#include <SdsDustSensor.h>

// const char * host = "192.168.178.21";
// const char * url = "/sensor";
// const int httpPort = 5000;
// const int delayAfterSuccessfulSendingMs = 5 * 1000;

const char * host = "api.sensor.community";
const char * url = "/v1/push-sensor-data/";
const int httpPort = 80;
const int delayAfterSuccessfulSendingMs = 70 * 1000;

/* WiFi settings */
const char * wifi_ssid = "WiFi network name";
const char * wifi_password = "WiFi network password";
WiFiClient client;

/* SDS011 Dust Sensor */
const int SDS_RX_PIN = D1;
const int SDS_TX_PIN = D2;
SdsDustSensor sds(SDS_RX_PIN, SDS_TX_PIN);

/* DHT22 Temperature and Humidity Sensor */
#define DHTTYPE    DHT22
DHT dht(D7, DHTTYPE);

String chipId;

void setup()
{
    // put your setup code here, to run once:
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    dht.begin();
    setupSDS();
    connectToWiFi();
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

void connectToWiFi()
{
    Serial.printf("Connecting to '%s'\n", wifi_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);
    if (WiFi.waitForConnectResult() == WL_CONNECTED)
    {
        Serial.print("Connected. IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Connection Failed!");
    }
}

void loop()
{
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    if (sendData(7, "humidity", humidity, "temperature", temperature))
    {
        delay(delayAfterSuccessfulSendingMs);
    }

    PmResult pm = sds.readPm();
    if (pm.isOk())
    {
        if (sendData(1, "P1", pm.pm10, "P2", pm.pm25))
        {
            delay(delayAfterSuccessfulSendingMs);
        }
    }
    else
    {
        Serial.println("SDS status: " + pm.statusToString());
    }
}

int sendData(int pin, String header1, float val1, String header2, float val2)
{
    if (client.connect(host, httpPort))
    {
        Serial.println("connection successfully: " + String(host));
    }
    else
    {
        Serial.println("connection failed");
        return 0;
    }

    Serial.println();

    String body = "{";
    body += "\"software_version\":\"0.1\",";
    body += "\"sensordatavalues\": [";
    body += "{\"value_type\":\"" + header1 + "\", \"value\":\"" + String(val1) + "\"},";
    body += "{\"value_type\":\"" + header2 + "\", \"value\":\"" + String(val2) + "\"}";
    body += "]}\r\n";

    String httpMessage = "POST " + String(url) + " HTTP/1.1\r\n";
    httpMessage += "Content-Type: application/json\r\n";
    httpMessage += "X-Pin: " + String(pin) + "\r\n";
    httpMessage += "Content-Length: " + String(body.length()) + "\r\n";
    httpMessage += "Host: " + String(host) + "\r\n";
    httpMessage += "X-Sensor: esp8266-" + chipId + "\r\n";
    httpMessage += "Connection: close\r\n\r\n";
    httpMessage += body;

    Serial.println("send message:");
    Serial.println(httpMessage);

    client.println(httpMessage);

    Serial.println("read response:");
    while(client)
    {
        Serial.print((char)client.read());
    }

    client.stop();

    Serial.println("\r\ndisconnected");

    return 1;
}