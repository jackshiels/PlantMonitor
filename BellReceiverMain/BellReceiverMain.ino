#include <Servo.h>

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
  if (Serial.available() > 0){
    incomingByte = Serial.read();
    Serial.println(incomingByte);
    if (incomingByte == '1'){
      Serial.println("Success!");
      // scan from 0 to 180 degrees
      for(angle = 10; angle < 180; angle++)  
      {                                  
        //servo.write(angle);               
        delay(15);                   
      } 
      // now scan back from 180 to 0 degrees
      for(angle = 180; angle > 10; angle--)    
      {                                
        //servo.write(angle);           
        delay(15);       
      } 
    }
  }
}