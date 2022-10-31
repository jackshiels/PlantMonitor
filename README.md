# Plant Monitor
This repo documents the process and resources used to develop a fully functioning plant monitor system using Arduinos, a DHT temperature and humidity sensor, and a moisture sensor instrument. Additionally, the project includes a humourous physical alert mechanism that lets the owner know their plant is low on water! Work on this project was conducted as a part of CASA0014, a module in the UCL Bartlett Connected Environments MSc course.

This project is split into three parts:
- Conceptualisation and experimentation
- Development and testing (includes a list of parts and an explanation of the code)
- Deployment into the CASA lab

# Conceptualisation and Experimentation
While some parts of this project were given via specification, others were drawn on through conceptualisation. Initially, three seperate ideas were floated. These were:
- Develop a mechanical base for the plant that rotates to align with the brightest sunlight spots in a room
- Added sensors and communication APIs to a group of plants that communicate their statuses with one other (a kind of shared intelligence!)
- Add a bell to the plant that is rung when the soil moisture dips below a certain threshold

![Concept art](https://github.com/jackshiels/PlantMonitor/blob/main/Images/sketches_composite.jpeg?raw=true)

The first idea was interesting, but would require a very powerful servo. Such a servo would be challenging to power from a small Arduino board. The second idea held promise, but coordination between students to build this plant network was not possible due to varying schedules. Lastly, the bell ringer idea was again hamstrung by the need for a powerful ringing servo motor, which would not be easy to integrate at the time.

The final idea was to extend the plant monitor tutorial to not only collect data, but also act on that data with a servo. This servo would power a flag on the plant that would wave when moisture dropped below a certain level (or a signal was sent to the device). Experimentation began with this idea in mind.

# Development and testing
This section details how to create the device, and what was done to build its functionality.

This build required:
- An Adafruit Huzzah with ESP8266 WiFi board
- A custom soldered shield for the Huzzah board
- An Arduino Nano
- A Raspberry Pi
- A DHT temperature and humidity sensor
- Four male-to-male wire connectors
- A SG90-HV servo
- Two open wires for the moisture sensor
- Two nails for moisture sensing
- A 3D printed case for the sensor

A breadboard mockup of the DHT sensor is shown below:

![Concept art](https://github.com/jackshiels/PlantMonitor/blob/main/Images/sensor_breadboard.jpeg?raw=true)

The final design is shown below. 

On the Adafruit Huzzah, the DHT sensor requires power (VCC), a ground pin (GND), and a data pin. Furthermore, several resistors were soldered onto a circuit that has two wires wrapped around a pair of nails. The circuit created by moisture within the soil passes through these resistors and gets read by the Huzzah.

Additionally, an Arduino Uno was added to handle the flag servo. This servo takes VCC, GND, and a single data pin to address servo rotation. A single pin was connected from the Tx on the Huzzah to the Rx on the Arduino Uno. Serial signals were routed through this connection to activate the servo on the Arduino Uno.

Lastly, a Raspberry Pi was added to capture MQTT data in a database and report via a web dashboard. This dashboard stack was composed of InfluxDB for data storage and management, Telegraf for MQTT data capture, and Grafana for the presentation of this captured data.

![Concept art](https://github.com/jackshiels/PlantMonitor/blob/main/Images/final_setup.jpeg?raw=true)

## Code

This repo has two main Arduino source files. These are:
- PlantMonitorMain.ino: manages the Huzzah, its WiFi, data capture, and serial communication with the Arduino Uno
- FlagReceiverMain: receives Huzzah signals via serial and controls the flag servo on the Arduino Uno

## PlantMonitorMain

PlantMonitorMain uses several libraries. These libraries are `<ESP8266WiFi.h>` for WiFi, `<ezTime.h>` for time capture on Arduino, `<PubSubClient.h>` for MQTT access, `<DHT.h>` for the DHT sensor, and a `"arduino_secrets.h"` script to hide private WiFi details. 

The setup method initialises the pins, opens serial connectivity, and starts the WiFi and MQTT networking methods. The main loop operates on a 10,000ms second delay, meaning it grabs DHT and moisture data once every ten seconds. Both moisture reading and DHT data gathering are abstracted into their own methods (`ReadMoisture()` and `ReadDHTValues()`). `ReadMoisture()` does a check for the moisture threshold, which is set at 40. Once below this number, it sends a serial value of `1` to the other Arduino to activate the 'Help!' flag.

```
void loop() {
  // handler for receiving requests to webserver
  server.handleClient();
  delay(10000);
  readMoisture();
  sendMQTT();

  // Send the Rx signal to the other Arduino
  client.loop();
}
```
Additionally, a callback function `callback` is provided to allow remote activation of the flag servo. This method can receive an MQTT signal at the topic `activateServo`, which is subscribed to in the `reconnect` method.

```
void callback(char* topic, byte* payload, unsigned int length) {
  // Receives a message from MQTT
  // Activates servo if character is 1
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    Serial.print(1);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}
```

## FlagReceiverMain

FlagReceiverMain is the script that is run on the Arduino Uno. The purpose of this script is to receive serial signals and move a servo in response. These serial signals are sent from the Huzzah board. The script relies on `<Servo.h>` to control the SG90-HV servo motor, which is activated when a single char of '1' is received through the Arduino's Rx pin. Once received, the servo is rotated 120 degrees and back again.

```
void loop() 
{ 
  // If the serial is set up
  if (Serial.available() > 0){
    // Read the byte from the Huzzah
    incomingByte = Serial.read();
    Serial.println(incomingByte);
    // If activated
    if (incomingByte == '1'){
      Serial.println("Success!");
      // scan from 0 to 120 degrees
      for(angle = 10; angle < 120; angle++)  
      {                                  
        servo.write(angle);               
        delay(15);                   
      } 
      // now scan back from 120 to 0 degrees
      for(angle = 120; angle > 10; angle--)    
      {                                
        servo.write(angle);           
        delay(15);       
      } 
    }
    else {
      Serial.println("0");
    }
  }
  else {
      Serial.println("0");
    }
}
```
