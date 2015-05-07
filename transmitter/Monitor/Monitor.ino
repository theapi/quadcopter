

#include <Wire.h>
#include "U8glib.h"

// Small ack payload for speed
typedef struct{
  uint8_t key;
  uint16_t val;
}
ack_t;
ack_t ack_payload;

typedef struct{
  int vcc_tx;
  int vcc_rx;
  int pps;
  int pps_lost;
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;

}
monitor_t;
monitor_t monitor;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
U8GLIB_SSD1306_128X64_2X u8g(U8G_I2C_OPT_NONE);

unsigned long comms_last = 0;
unsigned long display_last = 0;
unsigned long vcc_last = 0;
int vcc = 0;

void batterySetup()
{
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
}

long batteryVcc() {

  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}


void setup()
{
  batterySetup();
  Serial.begin(115200);
  u8g.setRot180();
  memset(&monitor, 0, sizeof(monitor_t));
  memset(&ack_payload, 0, sizeof(ack_t));

}

void loop()
{
  static byte ack_flag = 0;
  static byte data[3];
  
  unsigned long now = millis(); 
  
  while (Serial.available() > 0) {
    // Warn when monitor has lost comms with the TX.
    comms_last = now;
    byte c = Serial.read();

    if (ack_flag == 0 && c == 'A') {
      ack_flag = 1; 
    } else if (ack_flag == 1) {
      ack_flag = 2; 
      // next 3 bytes are the data
      data[0] = c;
    } else if (ack_flag == 2) {
      ack_flag = 3; 
      data[1] = c;
    } else if (ack_flag == 3) {
      ack_flag = 4;
      data[2] = c;  
    } else if (ack_flag == 4 && c == 'K') {
      ack_flag = 0; 
      
      ack_payload.key = data[0];
      ack_payload.val = (data[1] << 8) | data[2];
      
      switch (ack_payload.key) {
        case 255:
          monitor.pps = ack_payload.val;
          break;          
        case 0:
          monitor.vcc_rx = ack_payload.val;
          break;
        case 1:
          monitor.throttle = ack_payload.val;
          break;
        case 2:
          monitor.yaw = ack_payload.val;
          break;
        case 3:
          monitor.pitch = ack_payload.val;
          break;
        case 4:
          monitor.roll = ack_payload.val;
          break;
      }
      monitor.vcc_tx = vcc;
      
    } else {
      ack_flag = 0; 
    }
    
  }
  
  // Read the battery level
  if (now - vcc_last > 1000) {
    vcc = batteryVcc(); 
  }
  
  // Change the display if there is no serial data from the transmitter
  if (now - comms_last > 250) {
    memset(&monitor, 0, sizeof(monitor_t));
  }
  
  if (now - display_last > 50) {
    displayUpdate();
  }
 
}

void displayUpdate()
{
  // picture loop
  u8g.firstPage();
  do {
    draw();
  } while( u8g.nextPage() );
}

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here
  /*
  u8g.setFont(u8g_font_fub42n);
  u8g.setFontPosTop();
  u8g.setPrintPos(0, 0);
  u8g.print(monitor.throttle);

  u8g.setFont(u8g_font_fub14n);
  u8g.setFontPosTop();
  u8g.setPrintPos(105, 0);
  u8g.print(monitor.pps_lost);
*/

  u8g.setFont(u8g_font_fub11n);
  //u8g.setFont(u8g_font_unifont);
  u8g.setFontPosTop();
  

  u8g.setPrintPos(0, 0);
  u8g.print(monitor.throttle);
  u8g.setPrintPos(50, 0);
  u8g.print(monitor.yaw);
  
  u8g.setPrintPos(0, 20);
  u8g.print(monitor.pitch);
  u8g.setPrintPos(50, 20);
  u8g.print(monitor.roll);
  
  u8g.setPrintPos(90, 0);
  u8g.print(monitor.pps);
  
  u8g.setPrintPos(0, 52);
  u8g.print(monitor.vcc_tx);

  u8g.setPrintPos(80, 52);
  u8g.print(monitor.vcc_rx);
 
  
  
}

