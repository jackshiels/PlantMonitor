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

The following boards are required:
- An Adafruit Huzzah with ESP8266 WiFi board (with a custom soldered shield)
- An Arduino Nano
- A Raspberry Pi

The following sensor equipment is used:
- A DHT temperature and humidity sensor
- A SG90-HV servo
- Two open wires for the moisture sensor
- Two nails for moisture sensing
- 2x 10k Ohm resistors
- 1x 200 Ohm resistor
- 1x 100 Ohm resistor

Lastly, several miscellaneous items are required:
- A 3D printed case for the sensor
- Masking tape to secure the Huzzah in place
- Googly eyes for the plant (for fun!)

If you are completing this project with a Huzzah custom shield, you will need 4x male-to-male jumper cables. If you do not have a shield, you will need at least 11x jumper cables and a breadboard (depending on how well you can wire the resistor connections).

A breadboard mockup of the DHT sensor is shown below:

![Breadboard concept](https://github.com/jackshiels/PlantMonitor/blob/main/Images/sensor_breadboard.jpeg?raw=true)

A technical diagram is provided below, followed by a photograph of the finished project. 

On the Adafruit Huzzah, the DHT sensor requires power (VCC), a ground pin (GND), and a data pin. The DHT sensor requires a 10k Ohm resistor to pull up the data and VCC pins. The moisture sensor is built by connecting the first nail to digital output GPIO 013 on the Huzzah. This digital output sends a current to the first nail, while the second nail is connected to the analog input on pin A0. The sensor works by reading the current from the first nail as an analog signal in A0, as captured by the second nail. A lower level of resistance between the nails (due to increased moisture) will lead to higher analog readings on A0, while greater resistance (from less moisture) will lower the value.

Additionally, an Arduino Uno is used to control the flag motor. This servo takes VCC, GND, and a single data pin to address servo rotation. Furthermore, a single jumper cable is connected from the Tx port on the Huzzah to the Rx on the Arduino Uno. Serial signals are routed through this connection to activate the servo on the Arduino Uno.

Lastly, a Raspberry Pi is used to capture MQTT data in a database and report via a web dashboard. This dashboard stack is composed of InfluxDB for data storage and management, Telegraf for MQTT data capture, and Grafana for the presentation of this captured data. The Raspberry Pi should be on the same WiFi network as the Huzzah.

Below is a technical diagram of the design, followed by a photograph of the real-world components.

![Technical diagram](https://user-images.githubusercontent.com/43108815/201432659-ec17beb9-236b-40c9-b7e0-1f251710d830.png)

![Setup overview](https://github.com/jackshiels/PlantMonitor/blob/main/Images/final_setup.jpeg?raw=true)

## Code

This repo has two main Arduino source files. These are:
- PlantMonitorMain.ino: manages the Huzzah, its WiFi, data capture, and serial communication with the Arduino Uno
- FlagReceiverMain.ino: receives Huzzah signals via serial and controls the flag servo on the Arduino Uno

## PlantMonitorMain

[TODO: comment code to show you understand it.]

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

FlagReceiverMain is the script that is run on the Arduino Uno. The purpose of this script is to receive serial signals and move a servo in response. These serial signals are sent from the Huzzah board. The script relies on `<Servo.h>` to control the SG90-HV servo motor, which is activated when a single char of '1' is received through the Arduino's Rx pin. Originally, the servo was set to rotate 120 degrees and back again on a loop when moisture was low. However, some feedback was given to make the flag raise when moisture levels are poor, and lower again after sufficient watering. The new code is shown below. If the moisture level is low, a '1' serial signal is sent to raise the flag. Once moisture drops, a '0' signal is sent to lower it back to its original state. Below is a snippet of the core flag raising logic:

```
if (Serial.available() > 0){
    // Read the byte from the Huzzah
    incomingByte = Serial.read();
    Serial.println(incomingByte);
    // If activated
    if (incomingByte == '1'
    && angle == 10){
      Serial.println("Moisture level: low");
      // scan from 0 to 120 degrees
      for(angle = 10; angle < 120; angle++)  
      {                                  
        servo.write(angle);               
        delay(15);                   
      } 
    }
    else if (incomingByte == '0'
    && angle == 120){
      Serial.println("Moisture level: good");
      // Run the servo back down 120 degrees
      for(angle = 120; angle > 10; angle--)    
      {                                
        servo.write(angle);           
        delay(15);       
      } 
    }
```
## MQTT and Raspberry Pi setup

A Raspberry Pi was installed to capture and present sensor data. The Pi was set up to connect to a local network, and updated to the latest software. Three key software components were installed:
- InfluxDB, for storing time series data
- Telegraf, for capturing MQTT data
- Grafana, for presenting the data on a dashboard

InfluxDB was set up with a Telegraf data connector that could interpret the MQTT data. This data was transported into a bucket entitled 'mqtt-data'. Environment variables were written into the`/etc/profile/.profile` to ensure that Telegraf knew the address of the InfluxDB host, its token, and the organisation name wherein the bucket was located.

Grafana was then set up to grab data from the `mqtt-data` bucket, and a dashboard was designed:

![Grafana](https://github.com/jackshiels/PlantMonitor/blob/main/Images/grafana2.png?raw=true)

# Deployment into the CASA lab

![CASA Installation](https://github.com/jackshiels/PlantMonitor/blob/main/Images/final_installation.jpeg?raw=true)

The plant monitor system was deployed within the CASA lab on 31 October 2022. A USB dongle was connected to a power plug, from which the two Arduinos were powered. An initial moisture test was conducted by taking a dry plant as a base reading, then inserting the two nails into a glass of water. A minimum value of 9 was recorded, and a maximum of 300.

![MQTT](https://raw.githubusercontent.com/jackshiels/PlantMonitor/main/Images/mqtt.jpg)

The project was tested successfully using both the moisture threshold and an MQTT signal.

## Long-term support considerations

[TODO: go over LTS considerations.]

Try setting it up at home, and feel free to branch off this repo too!

Jack Shiels
