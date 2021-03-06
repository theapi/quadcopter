

#include <Wire.h>
#include "U8glib.h"
#include "comm.h"
#include "def.h"

#define PIN_SWITCH_A 17 // A3 (PC3 - PCINT11)
#define PIN_SWITCH_B 16 // A2 (PC2 - PCINT10)
#define PIN_SWITCH_C 15 // A1 (PC1 - PCINT9)
#define PIN_SWITCH_D 14 // A0 (PC0 - PCINT8)

typedef struct{
  int vcc_tx;
  int vcc_rx;
  int pps;
  int pps_lost;
  int throttle;
  int yaw;
  int pitch;
  int roll;

}
monitor_t;
monitor_t monitor;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
U8GLIB_SSD1306_128X64_2X u8g(U8G_I2C_OPT_NONE);

unsigned long comms_last = 0;
unsigned long display_last = 0;
unsigned long vcc_last = 0;
int vcc = 0;
uint8_t switches = 0; // bit flag

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
  
  // Push button switches
  pinMode(PIN_SWITCH_A, INPUT_PULLUP);
  pinMode(PIN_SWITCH_B, INPUT_PULLUP);
  pinMode(PIN_SWITCH_C, INPUT_PULLUP);
  pinMode(PIN_SWITCH_D, INPUT_PULLUP);
  
  // Do a couple of vcc readings, as the first may be junk.
  vcc = batteryVcc(); 
  vcc = batteryVcc(); 
}

void loop()
{
  static byte ack_flag = 0;
  static byte data[6];
  
  // http://www.labbookpages.co.uk/electronics/debounce.html
  static uint8_t debounce_switches[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  static unsigned long debounce_sample_last = 0;
  
  unsigned long now = millis(); 
  
  while (Serial.available() > 0) {
    // Warn when monitor has lost comms with the TX.
    comms_last = now;
    byte c = Serial.read();
    
    switch (ack_flag) {
      case 0:
        if (c == 'A') {
          ack_flag = 1; 
        }
        break;
        
      case 1: // key
        ack_flag = 2; 
        data[0] = c;
        break;
        
      case 2: // val high byte
        ack_flag = 3; 
        data[1] = c;
        break;
        
      case 3: // val low byte
        ack_flag = 4;
        data[2] = c; 
        break;
        
      case 4: // key (repeated)
        ack_flag = 5; 
        data[3] = c;
        break;
        
      case 5: // val high byte (repeated)
        ack_flag = 6; 
        data[4] = c;
        break;
        
      case 6: // val low byte (repeated)
        ack_flag = 7;
        data[5] = c; 
        break;
        
      case 7:
        ack_flag = 0; 
        if (c == 'K') {
          byte key = data[0];
          byte key_copy = data[3];
          if (key != key_copy) {
            break;
          }
          
          int val = (data[1] << 8) | data[2];
          int val_copy = (data[4] << 8) | data[5];
          if (val != val_copy) {
            break;
          }
          
          switch (key) {
            case NRF24_KEY_PPS:
              monitor.pps = val;
              break;          
            case NRF24_KEY_VCC:
              monitor.vcc_rx = val;
              break;
            case NRF24_KEY_THROTTLE:
              monitor.throttle = val;
              break;
            case NRF24_KEY_YAW:
              monitor.yaw = val;
              break;
            case NRF24_KEY_PITCH:
              monitor.pitch = val;
              break;
            case NRF24_KEY_ROLL:
              monitor.roll = val;
              break;
          }
          
          monitor.vcc_tx = vcc;
          
        }
        break;
        
      default:
        ack_flag = 0;
        break;
    }
    
  }
  
  // Read the battery level
  if (now - vcc_last > 1000) {
    vcc_last = now;
    //vcc = batteryVcc();

    // Running average
    vcc = vcc * 0.9 + batteryVcc() * 0.1;
  }
  
  // Change the display if there is no serial data from the transmitter
  if (now - comms_last > 250) {
    memset(&monitor, 0, sizeof(monitor_t));
    monitor.vcc_tx = vcc;
  }
  
  // Poll the switches 
  if (now - debounce_sample_last > 5) {
    // Debounce, then set the mode.
    debounce_sample_last = now;
    // Read all the switch pins at once.
    uint8_t port_c = PINC;

    // Shift the variable towards the most significant bit
    // and set the least significant bit to the current switch value
    for (uint8_t i = 0; i < 4; i++) {
      debounce_switches[i] <<= 1;
      bitWrite(debounce_switches[i], 0, bitRead(port_c, i));
      if (debounce_switches[i] == 0) {
        bitWrite(switches, i, 1);
      } else {
        bitWrite(switches, i, 0);
      }
    }
    
    /*
    debounce_switches[0] <<= 1;
    bitWrite(debounce_switches[0], 0, bitRead(port_c, PINC0));

    debounce_switches[1] <<= 1;
    bitWrite(debounce_switches[1], 0, bitRead(port_c, PINC1));

    debounce_switches[2] <<= 1;
    bitWrite(debounce_switches[2], 0, bitRead(port_c, PINC2));
    
    debounce_switches[3] <<= 1;
    bitWrite(debounce_switches[3], 0, bitRead(port_c, PINC3));

    if (debounce_switches[0] == 0) {
      bitWrite(switches, 0, 1);
    } else {
      bitWrite(switches, 0, 0);
    }
    
    if (debounce_switches[1] == 0) {
      bitWrite(switches, 1, 1);
    } else {
      bitWrite(switches, 1, 0);
    }
    
    if (debounce_switches[2] == 0) {
      bitWrite(switches, 2, 1);
    } else {
      bitWrite(switches, 2, 0);
    }
    
    if (debounce_switches[3] == 0) {
      bitWrite(switches, 3, 1);
    } else {
      bitWrite(switches, 3, 0);
    }
    */
    


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

