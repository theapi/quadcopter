
#include <SPI.h>
#include <Wire.h>
#include "U8glib.h"

#define PAYLOAD_BUFFER_LENGTH 64

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
ack_t payload_buffer[PAYLOAD_BUFFER_LENGTH]; // A temporary store the spi interrupts
volatile byte payload_buffer_head;
volatile byte payload_buffer_tail;


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
    ack_payload.key = (spi_buffer[0] << 8) | spi_buffer[1];
    ack_payload.val = (spi_buffer[2] << 8) | spi_buffer[3];
    payload_buffer[payload_buffer_head] = ack_payload;
  
    // Increment the index
    payload_buffer_head++;
    if (payload_buffer_head >= PAYLOAD_BUFFER_LENGTH) {
      payload_buffer_head = 0;
    }

  }
} 

void setup()
{
  Serial.begin(57600);
  
  memset(&monitor, 0, sizeof(monitor_t));
  memset(&ack_payload, 0, sizeof(ack_t));
  

}

void loop()
{
  static byte ack_flag = 0;
  static byte data[4];
  
  while (Serial.available() > 0) {
    byte c = Serial.read();

    if (ack_flag == 0 && c == 'A') {
      ack_flag = 1; 
    } else if (ack_flag == 1 && c == 'C') {
      ack_flag = 2; 
    } else if (ack_flag == 2 && c == 'K') {
      ack_flag = 3; 
    } else if (ack_flag == 3) {
      ack_flag = 4; 
      // next 4 bytes are the data
      data[0] = c;
    } else if (ack_flag == 4) {
      ack_flag = 5; 
      data[1] = c;
    } else if (ack_flag == 5) {
      ack_flag = 6; 
      data[2] = c;
    } else if (ack_flag == 6) {

      data[3] = c;

      ack_payload.key = (data[0] << 8) | data[1];
      ack_payload.val = (data[2] << 8) | data[3];
      

      switch (ack_payload.key) {
        case 255:
          monitor.pps = ack_payload.val;
          break;
        case 254:
          monitor.pps_lost = ack_payload.val;
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
      
      ++monitor.pitch; //tmp

      
    } 
  }
  
  // If head & tail are not in sync, process the next.
  //if (payload_buffer_head != payload_buffer_tail) {
  //  ack_t ack = payload_buffer[payload_buffer_tail];
    

  
  /*  
    // Increment the tail index
    payload_buffer_tail++;
    if (payload_buffer_tail >= PAYLOAD_BUFFER_LENGTH) {
      payload_buffer_tail = 0;
    }
    */
  //}
 
  
  
 // monitor.pps = ack_flag; //tmp
  
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
  u8g.print(monitor.throttle);

  u8g.setFont(u8g_font_fub14n);
  u8g.setFontPosTop();
  u8g.setPrintPos(105, 0);
  u8g.print(monitor.pps_lost);

  u8g.setFont(u8g_font_fub11n);
  u8g.setFontPosTop();
  
  u8g.setPrintPos(0, 52);
  u8g.print(monitor.pitch);

  u8g.setPrintPos(23, 52);
  u8g.print(monitor.roll);

  u8g.setPrintPos(90, 52);
  u8g.print(monitor.pps);
}

