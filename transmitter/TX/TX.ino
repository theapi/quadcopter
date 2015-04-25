/**
 * Totally inspired by iforce2d's "Cheap-ass quadcopter build"
 * https://www.youtube.com/playlist?list=PLzidsatoEzeieT03YQ6-LpO0bR1yfEZpx
 *
 * But this is even cheaper (if you happen to have a Hubsan X4 which you fried)
 *
 * Uses the Optimized fork of nRF24L01 for Arduino and Raspberry Pi 
 * https://github.com/TMRh20/RF24
 *
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define DEBUG 0

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#define PIN_CE  7
#define PIN_CSN 8
#define PIN_MONITOR_SS 4

#define PIN_THROTTLE A0
#define PIN_YAW      A1
#define PIN_PITCH    A2
#define PIN_ROLL     A3

RF24 radio(PIN_CE, PIN_CSN);

typedef struct{
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll;
  byte dial1;
  byte dial2;
  byte switches; // bitflag
}
tx_t;
tx_t tx_payload;

// Small ack payload for speed
typedef struct{
  uint8_t key;
  uint16_t val;
}
ack_t;
ack_t ack_payload;


byte pipe_tx[6] = TX_ADDRESS;
byte pipe_rx[6] = RX_ADDRESS;

unsigned long ppsLast = 0;
int ppsCounter = 0;
int pps = 0;
int ppsLost = 0;
int ppsLostCounter = 0;

unsigned long vcc_last = 0;

void resetPayload() 
{
  tx_payload.throttle = 0;
  tx_payload.yaw = 127;
  tx_payload.pitch = 127;
  tx_payload.roll = 127;
  tx_payload.dial1 = 0;
  tx_payload.dial2 = 0;
  tx_payload.switches = 0;
}

void setup() 
{
  Serial.begin(115200);
  
  if (DEBUG) {
    
    printf_begin();
    Serial.println("Begin");
  }
   
  resetPayload();
   
  radio.begin(); // does the equivelent of SPI.begin()
  radio.setChannel(80);
  radio.setPayloadSize(sizeof(tx_t));
  
  // For the slower data rate (further range) see https://github.com/TMRh20/RF24/issues/98
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(1);
  radio.enableAckPayload();
  
  // How long to wait between each retry, in multiples of 250us,
  // max is 15.  0 means 250us, 15 means 4000us.
  // count How many retries before giving up, max 15
  // 3 * 250 = 750 = time required for an 8 byte ack
  radio.setRetries(3, 1);
  
  radio.openWritingPipe(pipe_tx);
  radio.openReadingPipe(1, pipe_rx);
  
  if (DEBUG) {
    // Dump the configuration of the rf unit for debugging
    radio.printDetails();   
    Serial.println("End begin");
  }
  
  ack_payload.key = 253;
  ack_payload.val = 4000; //@todo tx battery level
  monitor_sendData();
  
  radio.startListening();
}


/****************************************************************************/

// Returns a corrected value for a joystick position that takes into account
// the values of the outer extents and the middle of the joystick range.
int joystickMapValues(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}

/****************************************************************************/


void loop(void)
{
  tx_payload.throttle = joystickMapValues( analogRead(PIN_THROTTLE), 100, 500, 900, false );
  tx_payload.yaw      = joystickMapValues( analogRead(PIN_YAW), 100, 500, 900, false );
  tx_payload.pitch    = joystickMapValues( analogRead(PIN_PITCH), 100, 500, 900, false );
  tx_payload.roll     = joystickMapValues( analogRead(PIN_ROLL), 100, 500, 900, false );
   
  if (DEBUG) {
    //printf("Now sending throttle %d\n\r ",tx_payload.throttle);
  }
  
  // Stop listening so we can talk.
  radio.stopListening();                                  
        
  // Take the time, and send it.  This will block until complete 
  unsigned long time = micros();  
  if (!radio.write( &tx_payload, sizeof(tx_t))) {
    // Got no ack.
    if (DEBUG) {
      //printf("no ack.\n\r"); 
      ++ppsLostCounter;
    }
  } else {
     if (!radio.available()) { 
       // blank payload
       if (DEBUG) {
         //printf("Blank Payload Received\n\r"); 
       }
     } else {
       
       if (radio.available() ){
          unsigned long tim = micros();
          radio.read( &ack_payload, sizeof(ack_t) );
          ppsCounter++;
          monitor_sendData();
          if (DEBUG) {
            printf("Got response %d, pps: %d , round-trip: %lu microseconds\n\r", ack_payload.val, pps, tim-time);
          }
          

          
        }

     }
  }
  
  unsigned long now = millis();
  if ( now - ppsLast > 250 ) {
    pps = ppsCounter;
    ppsLost = ppsLostCounter;
    ppsCounter = 0;
    ppsLostCounter = 0;
    ppsLast = now;
    if (DEBUG) {
      printf("pps: %d, lost: %d \n\r", pps, ppsLost);
    }
    ack_payload.key = 255;
    ack_payload.val = (pps << 2); // Multiply by 4 (fast)
    monitor_sendData();
  } 
  
  if (now - vcc_last > 3000) {
    vcc_last = now;
    ack_payload.key = 253;
    ack_payload.val = 4000; //@todo tx battery level
    monitor_sendData();
  }

}

void monitor_sendData()
{
  if (!DEBUG) {
    byte data[5];
  
    data[0] = 'A';
    data[1] = ack_payload.key;
    data[2] = (ack_payload.val >> 8);
    data[3] = ack_payload.val;
    data[4] = 'K';
    
    Serial.write(data, 5);
    //Serial.flush();
  }
}



