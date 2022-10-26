#include <ESP8266WiFi.h>
 
 const char* ssid = "5GEE-Router-893N";
 const char* password = "Shellfish678*%*";
 const char* host = "apple.com";

void setup()
{
  // Open the serial monitor
  Serial.begin(115200);
  delay(100);

  // Connect to WiFi
  Serial.println();
  Serial.println("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("-");
  }
  Serial.print("*");

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());
}

void loop()
{
  delay(5000);

  Serial.println("-------------------------------");
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

    // We now create a URI for the request
  String url = "/index.html";
  Serial.print("Requesting URL: ");
  Serial.println(host + url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  delay(500);

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}