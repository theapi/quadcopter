

int PIN_THROTTLE = A0;    // select the input pin for the potentiometer
int PIN_MOTOR = 6;      // select the pin for the LED


void setup() {
 //Serial.begin(115200);
}

void loop() {
 
  
  byte pwm = joystickMapValues( analogRead(PIN_THROTTLE), 0, 512, 1024, false );
  analogWrite(PIN_MOTOR, pwm);   

  //Serial.println(pwm);  
  //delay(250);
}

int joystickMapValues(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}
