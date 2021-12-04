# airqstation

Firmware for air quality station that is described here https://sensor.community/en/sensors/airrohr/

It connects to WiFi network amd sends data (pm2.5 level, pm10 level, temperature, humidity) to the server (http://api.sensor.community/v1/push-sensor-data/) every 70 seconds.

The board components:
- NodeMCU (ESP8266)
- SDS011 Dust Sensor
- DHT22 Temperature and Humidity Sensor
