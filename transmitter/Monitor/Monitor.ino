
#include <SPI.h>
#include <Wire.h>
#include "U8glib.h"


typedef struct{
  uint16_t key;
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

unsigned long fpsLast = 0;
int fpsCounter = 0;
int fps = 0;

char spi_buffer[4];
volatile byte spi_buffer_index;
ack_t payload_buffer[64]; // A temporary store the spi interrupts
volatile byte payload_buffer_index;

//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
U8GLIB_SSD1306_128X64_2X u8g(U8G_I2C_OPT_NONE);

// SPI interrupt routine
ISR (SPI_STC_vect)
{
  
  // only listen if ss is low (arunino pin 8)
  uint8_t port_b = PINB;
  if (bitRead(port_b, PINB0)) {
    return;
  }
  
  byte c = SPDR;  // grab byte from SPI Data Register

  // add to buffer 
  if (spi_buffer_index < sizeof spi_buffer) {
    spi_buffer[spi_buffer_index++] = c;
  }
  
  if (spi_buffer_index > 3) {
    // full ack_payload
    spi_buffer_index = 0; 
    
    // add to the buffer to be processed in loop
    if (payload_buffer_index < sizeof payload_buffer) {
      ack_payload.key = (spi_buffer[0] << 8) | spi_buffer[1];
      ack_payload.val = (spi_buffer[2] << 8) | spi_buffer[3];
      payload_buffer[payload_buffer_index++] = ack_payload;
    }
  }
} 

void setup()
{
  memset(&monitor, 0, sizeof(monitor_t));
  memset(&ack_payload, 0, sizeof(ack_t));
  
  // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);
  
  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  
  // now turn on interrupts
  SPI.attachInterrupt();
}

void loop()
{
  
  if (payload_buffer_index > 0) {
    // process waiting payloads
   
    // ...
    
  }
  
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

