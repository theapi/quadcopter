
#include <SPI.h>
#include <Wire.h>
#include "U8glib.h"

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

unsigned long fpsLast = 0;
int fpsCounter = 0;
int fps = 0;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
U8GLIB_SSD1306_128X64_2X u8g(U8G_I2C_OPT_NONE);

void setup()
{
  memset(&monitor, 0, sizeof(monitor_t));
}

void loop()
{
  
  ++monitor.pps_lost;
  monitor.pps = fps; //tmp
  
  unsigned long now = millis();
  if ( now - fpsLast > 1000 ) {
    fps = fpsCounter;
    fpsCounter = 0;
    fpsLast = now;
    
    
  }
  
  displayUpdate();
  
  //delay(250); // tmp
}

void displayUpdate()
{
  ++fpsCounter;
  // picture loop
  u8g.firstPage();
  do {
    draw();
  } while( u8g.nextPage() );
}

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here
  u8g.setFont(u8g_font_fub42n);
  u8g.setFontPosTop();
  u8g.setPrintPos(0, 0);
  u8g.print(monitor.pps);

  u8g.setFont(u8g_font_fub14n);
  u8g.setFontPosTop();
  u8g.setPrintPos(105, 0);
  u8g.print(monitor.pps_lost);

  u8g.setFont(u8g_font_fub11n);
  u8g.setFontPosTop();

  u8g.setPrintPos(90, 52);
  u8g.print(monitor.pps_lost);
}

