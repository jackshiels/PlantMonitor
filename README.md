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

Lastly, a Raspberry Pi was added to capture MQTT data in a database and report via a web dashboard. This dashboard was composed of InfluxDB for data storage management, Telegraf for MQTT data capture, and Grafana for presentation of this captured data.

![Concept art](https://github.com/jackshiels/PlantMonitor/blob/main/Images/final_setup.jpeg?raw=true)

