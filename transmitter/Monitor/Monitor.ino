
#include <SPI.h>
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

void setup()
{
  Serial.begin(115200);
  
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
    } else if (ack_flag == 1 && c == 'C') {
      ack_flag = 2; 
    } else if (ack_flag == 2 && c == 'K') {
      ack_flag = 3; 
    } else if (ack_flag == 3) {
      ack_flag = 4; 
      // next 3 bytes are the data
      data[0] = c;
    } else if (ack_flag == 4) {
      ack_flag = 5; 
      data[1] = c;
    } else if (ack_flag == 5) {
      data[2] = c;

      ack_payload.key = data[0];
      ack_payload.val = (data[1] << 8) | data[2];
      

      switch (ack_payload.key) {
        case 255:
          monitor.pps = ack_payload.val;
          break;
        case 253:
          monitor.vcc_tx = ack_payload.val;
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
      ack_flag = 0;
      
    } 
  }
  
  
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

