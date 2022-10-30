#include <Servo.h>

// Declare servo class and incoing serial value
Servo servo;
int angle = 10;
char incomingByte = "0";

void setup() {
  servo.attach(13);
  servo.write(angle);
  Serial.begin(115200);
}

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

// Some code modified from https://www.electronics-lab.com/project/using-sg90-servo-motor-arduino/