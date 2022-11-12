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

On the Adafruit Huzzah, the DHT sensor requires power (VCC), a ground pin (GND), and a data pin. The DHT sensor requires a 10k Ohm resistor to pull up the data and VCC pins [[1]](#1). The moisture sensor is built by connecting the first nail to digital output GPIO 013 on the Huzzah. This digital output sends a current to the first nail, while the second nail is connected to the analog input on pin A0. The sensor works by reading the current from the first nail as an analog signal in A0, as captured by the second nail [[2]](#2). A lower level of resistance between the nails (due to increased moisture) will lead to higher analog readings on A0, while greater resistance (from less moisture) will lower the value.

![Nails](https://github.com/jackshiels/PlantMonitor/blob/main/Images/nails.jpg?raw=true)

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

NOTE: you'll need to create an arduino_secrets.h file with the following variables and place it in the same folder:

```
#define SECRET_SSID "your ssid";
#define SECRET_PASS "your WiFi password";
#define MQTT_SERVER "your MQTT broker address";
#define MQTT_PORT the port of your broker;
#define SECRET_MQTTUSER "your MQTT user";
#define SECRET_MQTTPASS "your mqtt password";
```

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

The `sendMQTT()` uses a `PubsubClient` object to push data to the CASA MQTT server, based on the data topic (e.g., moisture or humidity). I also set up my own Mosquitto MQTT broker to capture data from my home plant installation. For details on how to set up your own MQTT server on Raspberry Pi, please see [[3]](#3).  

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

FlagReceiverMain is the script that is run on the Arduino Uno. This script was partially adapted from some sample code [[4]](4). The purpose of this script is to receive serial signals and move a servo in response. These serial signals are sent from the Huzzah board. The script relies on `<Servo.h>` to control the SG90-HV servo motor, which is activated when a single char of '1' is received through the Arduino's Rx pin. Originally, the servo was set to rotate 120 degrees and back again on a loop when moisture was low. However, some feedback was given to make the flag raise when moisture levels are poor, and lower again after a sufficient watering. The new code is shown below. If the moisture level is low, a '1' serial signal is sent to raise the flag. Once moisture drops, a '0' signal is sent to lower it back to its original state. Below is a snippet of the core flag raising logic:

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

A Raspberry Pi should be installed to capture and present sensor data. The Pi must be connected to the same local network as the Huzzah, and updated to the latest software. Three key software components should be installed:
- InfluxDB, for storing time series data
- Telegraf, for capturing MQTT data
- Grafana, for presenting the data on a dashboard

InfluxDB can be installed via SSH terminal access to the Raspberry Pi. When setting up, ensure your organisation is set to something memorable (in this case, `telegraf` was used). Ideally, you should set up InfluxDB with a Raspberry Pi template. This can be done in the 'Settings' page. Next, create a bucket entitled `mqtt-data`. In the 'Load Data' screen, elect to create a Telegraf API token. Save this for the next step.

InfluxDB should be set up with a Telegraf data connector that can interpret the MQTT data. This data should be transported into the 'mqtt-data' bucket. After installing Telegraf, set up its environment variables for access to InfluxDB. These environment variables should include:

```
export INFLUX_HOST=[your Pi IP address]:8086
export INFLUX_ORG=some organisation name
export INFLUX_TOKEN=[the token that InfluxDB provided earlier]
```

Environment variables should ideally be written into the `/etc/profile/` directory to ensure that Telegraf knows the address of the InfluxDB host, its token, and the organisation name wherein the bucket is located on device startup. You can edit this document by opening it with the following command: `sudo nano /etc/profile/`.

Start Telegraf with the following command: `telegraf --config [my Raspberry Pi IP address]:8086/api/v2/telegrafs/[ID provided by InfluxDB]`.

Grafana should then be configured to grab data from the `mqtt-data` bucket. After installing Grafana, create a data source that references the localhost IP `127.0.0.1:8086`. Ensure you are grabbing from the `mqtt-data` bucket. From here, a dashboard can be designed with the appropriate queries:

![Grafana](https://github.com/jackshiels/PlantMonitor/blob/main/Images/grafana2.png?raw=true)

Note: some of these instructions were adapted from [[5]](#5).

# Deployment into the CASA lab

![CASA Installation](https://github.com/jackshiels/PlantMonitor/blob/main/Images/final_installation.jpeg?raw=true)

The plant monitor system was deployed within the CASA lab on 31 October 2022. A USB dongle was connected to a power plug, from which the two Arduinos were powered. An initial moisture test was conducted by taking a dry plant as a base reading, then inserting the two nails into a glass of water. A minimum value of 9 was recorded, and a maximum of 300.

![MQTT](https://raw.githubusercontent.com/jackshiels/PlantMonitor/main/Images/mqtt_broker.png)

The project was tested successfully using both the moisture threshold and an MQTT signal.

## Long-term support considerations

The plant monitor was secured onto the plant using masking tape, but this will need to be replaced at a later stage to avoid degredation. Similarly, the nails will inevitably rust, and a manufactured moisture sensor component may be better for long-term support. Lastly, the jumper cable to handle serial communication between the two boards can break if mishandled. I had to replace several jumper cables while testing. A good solution for this problem would be to buy a larger board with a 5V output for the servo, consolidating everything into a single Arduino/ESP8266.

Try setting it up at home, and feel free to branch off this repo too!

Jack Shiels

# References
<a id="1">[1]</a>
Yapa, M. T. (2021). <i>DHT11 & DHT22 Sensors Temperature using Arduino</i>. Available at: [https://create.arduino.cc/projecthub/MinukaThesathYapa/dht11-dht22-sensors-temperature-using-arduino-b7a8d6](https://create.arduino.cc/projecthub/MinukaThesathYapa/dht11-dht22-sensors-temperature-using-arduino-b7a8d6) (Accessed: 12 November 2022).

<a id="2">[2]</a>
Tucker, R. (2017). <i>Moisture detection with two nails</i>. Available at: [https://www.instructables.com/Moisture-Detection-With-Two-Nails/](https://www.instructables.com/Moisture-Detection-With-Two-Nails/) (Accessed: 12 November 2022).

<a id="3">[3]</a>
Random Nerd Tutorials (2022). <i>Install Mosquitto MQTT broker on Raspberry Pi</i>. Available at: [https://randomnerdtutorials.com/how-to-install-mosquitto-broker-on-raspberry-pi/](https://randomnerdtutorials.com/how-to-install-mosquitto-broker-on-raspberry-pi/) (Accessed: 13 November 2022).

<a id="4">[4]</a>
Koumaris, N. (2022). <i>Using the SG90 servo motor with an arduino</i>. Available at: [https://www.electronics-lab.com/project/using-sg90-servo-motor-arduino/](https://www.electronics-lab.com/project/using-sg90-servo-motor-arduino/) (Accessed: 12 November 2022).

<a id="5">[5]</a>
UCL CASA (2022). <i>Plant-Monitor</i>. Available at: [https://workshops.cetools.org/codelabs/CASA0014-2-Plant-Monitor/#0](https://workshops.cetools.org/codelabs/CASA0014-2-Plant-Monitor/#0) (Accessed: 13 November 2022).
