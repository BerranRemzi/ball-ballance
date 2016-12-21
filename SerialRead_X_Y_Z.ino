/*
  Reading a serial ASCII-encoded string.

 This sketch demonstrates the Serial parseInt() function.
 It looks for an ASCII string of comma-separated values.
 It parses them into ints, and uses those to fade an RGB LED.
 */
#define Xpin 6
#define Ypin 3
 
#include <Servo.h>

Servo Xservo;
Servo Yservo;

void setup() {
  // initialize serial:
  Serial.begin(115200);
  
  // make the pins outputs:
  Xservo.attach(Xpin);
  Yservo.attach(Ypin);
  Xservo.write(90);
  Yservo.write(90);
  delay(2000);
}

void loop() {
  
  // if there's any serial available, read it:
  while (Serial.available() > 0) {

    // look for the next valid integer in the incoming serial stream:
    int Xval = Serial.parseInt();
    // do it again:
    int Yval = Serial.parseInt();

    // look for the newline. That's the end of your
    // sentence:
    if (Serial.read() == '\n') {
      // constrain the values to 0 - 255 and invert
      // if you're using a common-cathode LED, just use "constrain(color, 0, 255);"
      //Xval = map(Xval, 0, 255, 120, 180);
      //Yval = map(Yval, 0, 255, 180, 120);

      // fade the red, green, and blue legs of the LED:
      Xservo.write(Xval);
      Yservo.write(Yval);

      // print the three numbers in one string as hexadecimal:
      //Serial.print(Xval, HEX);
      //Serial.println(Yval, HEX);
      //delayMicroseconds(10);
    }
  }
}
