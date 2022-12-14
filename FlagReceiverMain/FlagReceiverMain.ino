#include <Servo.h>

// Declare servo class and incoming serial value
Servo servo;
int angle = 10;
bool up = false;
char incomingByte = "0";

// Manages the servo on the Arduino Uno
void setup() {
  // Attach the servo to port 13 and write the initial angle
  servo.attach(13);
  servo.write(angle);
  Serial.begin(115200);
}

// Checks for input serial signals and moves the flag
void loop() 
{ 
  Serial.println(angle);
  // If the serial is set up
  if (Serial.available() > 0){
    // Read the byte from the Huzzah
    incomingByte = Serial.read();
    Serial.println(incomingByte);
    // 1 = activates flag upward, 0 = activates flag downward
    // If told to raise and already lowered
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
    // If told to lower and already raised
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
    else {
      Serial.println("No signal compatible");
    }
  }
  else {
      Serial.println("No signal detected via Rx");
    }
}

// Some code modified from https://www.electronics-lab.com/project/using-sg90-servo-motor-arduino/