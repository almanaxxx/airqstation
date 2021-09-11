#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char * wifi_ssid = "SpiderNet";
const char * wifi_password = "81880728550737648282";

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(D7, DHTTYPE);
//ESP8266WebServer server(8080);
WiFiServer server(80);

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  connectToWiFi();
}

//void setUpWebServer() {
//  server.on("/", handleWebServerRequest);
//  server.begin();
//}

void connectToWiFi() {
  Serial.printf("Connecting to '%s'\n", wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
    server.begin();
    //setUpWebServer();
  } else {
    Serial.println("Connection Failed!");
  }
}


  // Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
String header;

void loop() {
  digitalWrite(LED_BUILTIN, LOW);

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            float humidity = dht.readHumidity();
            float temperature = dht.readTemperature();

            client.println("<!DOCTYPE html><html>");
            client.println("<title>Munties Meteostation</title>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println("</style></head>");

            client.println("<body>");
            client.println("<h1>Welcome to Munties Meteostation!");
            client.println("<h3>Humidity: ");
            client.println(humidity);
            client.println("</h3>");
            client.println("<h3>Temperature: ");
            client.println(temperature);
            client.println("</h3>");
            client.println("</body>");
            client.println("</html>");
            client.println();
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

/*
void handleWebServerRequest() {
  Serial.println("------------------Somebody requested me");
  String message = "";
  message += "<!DOCTYPE html>";
  message += "<html>";
  message += "<head>";
  message += "<title>Meteostation</title>";
  message += "</head>";
  message += "<body>";
  message += "<h1>Hello World, I am a meteostation!</h1>";
  message += "</body>";
  message += "</html>";
  server.send(200, "text/html", message);
}*/
