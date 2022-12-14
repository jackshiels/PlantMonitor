#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ezTime.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include "arduino_secrets.h"
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

/*
NOTE: you'll need to create an arduino_secrets.h file and fill in the following const:
#define SECRET_SSID "your ssid";
#define SECRET_PASS "your WiFi password";
#define MQTT_SERVER "your MQTT broker address";
#define MQTT_PORT the port of your broker;
#define SECRET_MQTTUSER "your MQTT user";
#define SECRET_MQTTPASS "your mqtt password";
*/

// Sensors - DHT22 and Nails
uint8_t DHTPin = 12;        // on Pin 12
uint8_t soilPin = 0;      // ADC or A0 pin on Huzzah
float Temperature;
float Humidity;
int Moisture = 1; // initial value just in case web page is loaded before readMoisture called
int sensorVCC = 13;
int blueLED = 2;
DHT dht(DHTPin, DHTTYPE);   // Initialize DHT sensor.
int Tx = 16;

// Hidden variables for WiFi and MQTT
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqttuser = SECRET_MQTTUSER;
const char* mqttpass = SECRET_MQTTPASS;
const int mqttport = MQTT_PORT;
const char* mqtt_server = MQTT_SERVER;

// Connectivity variables
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// Date and time
Timezone GB;

void setup() {
  // LED light setup for a push command from MQTT to turn on and off
  pinMode(BUILTIN_LED, OUTPUT);     
  digitalWrite(BUILTIN_LED, HIGH);  

  // Set up the outputs to control the soil sensor
  // switch and the blue LED for status indicator
  // First: power for sensor
  pinMode(sensorVCC, OUTPUT); 
  digitalWrite(sensorVCC, LOW);
  pinMode(blueLED, OUTPUT); 
  digitalWrite(blueLED, HIGH);

  // open serial connection for debug info
  Serial.begin(115200);
  delay(100);

  // start DHT sensor and begin reading DHT
  pinMode(DHTPin, INPUT);
  dht.begin();

  // run initialisation functions for the WiFi and web server
  startWifi();
  startWebserver();
  syncDate();

  // start MQTT server
  client.setServer(mqtt_server, mqttport);
  client.setCallback(callback);
}

// Receives web server requests and reads sensor data every second
void loop() {
  // handler for receiving requests to webserver
  server.handleClient();
  delay(1000);

  // Get the moisture levels and DHT data
  readMoisture();
  sendMQTT();
  client.loop();
}

// Checks the resistance level between the two nails, based on an analog reading
// of A0
void readMoisture(){
  // Requests the moisture level
  // power the nail to drive an electrical current through the soil
  digitalWrite(sensorVCC, HIGH);
  digitalWrite(blueLED, LOW);
  delay(100);

  // read the electrical value from the sensor by grabbing its analog value
  Moisture = analogRead(soilPin);   

  // stop power to the nail
  digitalWrite(sensorVCC, LOW);  
  digitalWrite(blueLED, HIGH);
  delay(100);
}

// Connects to the WiFi network specified in arduino_secrets.h
void startWifi() {
  // We start by connecting to a WiFi network
  WiFi.begin(ssid, password);

  // check to see if connected or wait until you are
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void syncDate() {
  // get real date and time and set for UK time
  waitForSync();
  GB.setLocation("Europe/London");
}

// Loads a local web server where you can check sensor values
void startWebserver() {
  // When connected and IP address obtained start HTTP server
  // Sets delegate functions for server actions of GET
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
}

// Gets the DHT values and sends them, along with moisture, to the
// MQTT server specified in arduino_secrets.h
void sendMQTT() {
  // Check for a valid connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Get additional sensor values
  ReadDHTValues();

  // Sends the values of the temperature
  snprintf (msg, 50, "%.1f", Temperature);
  client.publish("student/CASA0014/plant/ucfnhie/temperature", msg);

  // Sends the values of the humidity
  snprintf (msg, 50, "%.0f", Humidity);
  client.publish("student/CASA0014/plant/ucfnhie/humidity", msg);

  // Moisture = analogRead(soilPin);   // moisture read by readMoisture function
  snprintf (msg, 50, "%.0i", Moisture);
  client.publish("student/CASA0014/plant/ucfnhie/moisture", msg);
}

// Reads from the DHT sensor
void ReadDHTValues(){
  // Get the temperature and humidity DHT values
  Temperature = dht.readTemperature();
  Humidity = dht.readHumidity();
}

// Activates if an MQTT signal is sent with the activateServo command (can be 0 or 1).
// Used to test the servo.
void callback(char* topic, byte* payload, unsigned int length) {
  // Receives a message from MQTT
  // Activates servo if character is 1
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    Serial.print(1);
  }
  else if ((char)payload[0] == '0'){
    Serial.print(0);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

// Connects to MQTT and handles the callback delegate for MQTT input signals.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect with clientID, username and password
    if (client.connect(clientId.c_str(), mqttuser, mqttpass)) {
      // ... and resubscribe
      client.subscribe("student/CASA0014/plant/ucfnhie/activateServo");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Performs the calculations for the web server page
void handle_OnConnect() {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity
  server.send(200, "text/html", SendHTML(Temperature, Humidity, Moisture));
}

// A 404 error message web page for the server
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

// Creates the HTML frontend for the web server
String SendHTML(float Temperaturestat, float Humiditystat, int Moisturestat) {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP8266 DHT22 Report</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Flag Plant Monitor Data</h1>\n";

  ptr += "<p>Temperature: ";
  ptr += (int)Temperaturestat;
  ptr += " C</p>";
  ptr += "<p>Humidity: ";
  ptr += (int)Humiditystat;
  ptr += "%</p>";
  ptr += "<p>Moisture: ";
  ptr += Moisturestat;
  ptr += "</p>";
  ptr += "<p>Sampled on: ";
  ptr += GB.dateTime("l,");
  ptr += "<br>";
  ptr += GB.dateTime("d-M-y H:i:s T");
  ptr += "</p>";

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}