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
#include <EasyTransfer.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "comm.h"
#include "def.h"


#define DEBUG 0

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#define PIN_CE  7
#define PIN_CSN 8

#define PIN_THROTTLE A0
#define PIN_YAW      A1
#define PIN_ROLL     A2
#define PIN_PITCH    A3

#define PIN_SWITCH_A 2 // (PD2 - PCINT18)
#define PIN_SWITCH_B 3 // (PD3 - PCINT19)
#define PIN_SWITCH_C 4 // (PD4 - PCINT20)
#define PIN_SWITCH_D 5 // (PD5 - PCINT21)
#define PIN_SWITCH_E 6 // (PD6 - PCINT22)

RF24 radio(PIN_CE, PIN_CSN);

typedef struct{
  int throttle;
  int yaw;
  int pitch;
  int roll;
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

EasyTransfer et; 
comm_t et_data;

byte pipe_tx[6] = TX_ADDRESS;
byte pipe_rx[6] = RX_ADDRESS;

unsigned long ppsLast = 0;
int ppsCounter = 0;
int pps = 0;
int ppsLost = 0;
int ppsLostCounter = 0;

uint16_t throttle;
uint16_t yaw;
uint16_t pitch; 
uint16_t roll; 

uint8_t switches = 0; // bit flag

void resetPayload() 
{
  tx_payload.throttle = 0;
  tx_payload.yaw = 1500;
  tx_payload.pitch = 1500;
  tx_payload.roll = 1500;
  tx_payload.switches = 0;
}

void setup() 
{
  // Push button switches
  pinMode(PIN_SWITCH_A, INPUT_PULLUP);
  pinMode(PIN_SWITCH_B, INPUT_PULLUP);
  pinMode(PIN_SWITCH_C, INPUT_PULLUP);
  pinMode(PIN_SWITCH_D, INPUT_PULLUP);
  pinMode(PIN_SWITCH_E, INPUT_PULLUP);
  
  Serial.begin(115200);
  //start the library, pass in the data details and the name of the serial port.
  et.begin(details(et_data), &Serial);
  
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
  
  //ack_payload.key = 253;
  //ack_payload.val = 4000; //@todo tx battery level
  //monitor_sendData();
  
  radio.startListening();
}


/****************************************************************************/

// Returns a corrected value for a joystick position that takes into account
// the values of the outer extents and the middle of the joystick range
// and maps it to the range the RX expects.
int joystickMapValues(int val, int lower, int middle, int upper, byte invert)
{
  val = constrain(val, lower, upper);
  
  if ( val < middle ) {
    val = map(val, lower, middle, 0, 499);
  } else {
    val = map(val, middle, upper, 500, 1000);
  }

  if (invert) {
    val = 1000 - val; 
  }
  
  // RX expects values between 1000 & 2000
  return val + 1000; 
}

/****************************************************************************/


void loop(void)
{
  // http://www.labbookpages.co.uk/electronics/debounce.html
  static uint8_t debounce_switches[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static unsigned long debounce_sample_last = 0;
 
  
  throttle = joystickMapValues(analogRead(PIN_THROTTLE), 125, 515, 1000, 0);
  yaw      = joystickMapValues(analogRead(PIN_YAW), 10, 517, 1000, 0);
  pitch    = joystickMapValues(analogRead(PIN_PITCH), 0, 513, 1023, 1);
  roll     = joystickMapValues(analogRead(PIN_ROLL), 0, 507, 1023, 1);
  

  if (DEBUG) {
    //printf("val: %d\n\r ",tx_payload.throttle);
  }
  
  // Stop listening so we can talk.
  radio.stopListening();                                  
        
  tx_payload.throttle = throttle;
  tx_payload.yaw = yaw;
  tx_payload.pitch = pitch;
  tx_payload.roll = roll;
  tx_payload.switches = switches;
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
          //unsigned long tim = micros();
          ack_t ack_payload;
          memset(&ack_payload, 0, sizeof(ack_t));
          radio.read( &ack_payload, sizeof(ack_t) );
          ppsCounter++;
          
          // sanity check
          if ( (ack_payload.key >= NRF24_KEY_VCC) && (ack_payload.key <= NRF24_KEY_ROLL)) {
            comm_t comm_payload;
            comm_payload.key = ack_payload.key;
            comm_payload.val = ack_payload.val;
            comm_push(comm_payload);
  
            if (DEBUG) {
              printf("Got key %d, val: %d \n\r", ack_payload.key, ack_payload.val);
            }
          } else if (DEBUG) {
            printf("Sanity failed: %d, val: %d \n\r", ack_payload.key, ack_payload.val);
          }

        }

     }
  }
  
  unsigned long now = millis();
  
  // Poll the switches 
  if (now - debounce_sample_last > 5) {
    // Debounce, then set the mode.
    debounce_sample_last = now;
    // Read all the switch pins at once.
    uint8_t port_d = PIND;

    // Shift the variable towards the most significant bit
    // and set the least significant bit to the current switch value
    for (uint8_t i = 0; i < 5; i++) {
      uint8_t pin = i + 2; // PD2 -> PD7
      debounce_switches[i] <<= 1;
      bitWrite(debounce_switches[i], 0, bitRead(port_d, pin));
      if (debounce_switches[i] == 0) {
        bitWrite(switches, i, 1);
      } else {
        bitWrite(switches, i, 0);
      }
    }
    
  }
  
  if ( now - ppsLast > 250 ) {
    pps = ppsCounter;
    ppsLost = ppsLostCounter;
    ppsCounter = 0;
    ppsLostCounter = 0;
    ppsLast = now;
    if (DEBUG) {
      //printf("pps: %d, lost: %d \n\r", pps, ppsLost);
    }

    comm_t comm_payload;
    comm_payload.key = NRF24_KEY_PPS;
    comm_payload.val = (pps << 2); // Multiply by 4 (fast)
    comm_push(comm_payload);

  } else {
    monitor_setLocalData(); 
  }
  
  monitor_sendData();

}

void monitor_sendData()
{
  if (!DEBUG) {
    //byte data[5];
  
    while (!comm_empty()) {
      comm_t payload = comm_shift();
      et_data.key = payload.key;
      et_data.val = payload.val;
      //send the data
      et.sendData();
  
      /*
      data[0] = 'A';
      data[1] = payload.key;
      data[2] = (payload.val >> 8);
      data[3] = payload.val;
      data[4] = 'K';

      Serial.write(data, 5);
      //Serial.flush();
      */
      
    }
  }
}

void monitor_setLocalData() {
  static uint8_t i = 5; 

  comm_t payload;
  payload.key = i;
  switch (i) {
    case NRF24_KEY_THROTTLE:
      payload.val = throttle; 
      break;
    case NRF24_KEY_YAW:
      payload.val = yaw; 
      break;
    case NRF24_KEY_PITCH:
      payload.val = pitch; 
      break;
    case NRF24_KEY_ROLL:
      payload.val = roll; 
      break;
  } 
  
  comm_push(payload);
      
  if (++i > 8) {
    i = 5; 
  }
  
}

