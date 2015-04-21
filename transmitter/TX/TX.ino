/**
 * Totally inspired by iforce2d's "Cheap-ass quadcopter build"
 * https://www.youtube.com/playlist?list=PLzidsatoEzeieT03YQ6-LpO0bR1yfEZpx
 *
 * But this is even cheaper (if you happen to have a Hubsan X4 which you fried)
 *
 * Uses the Optimized fork of nRF24L01 for Arduino and Raspberry Pi 
 * https://github.com/TMRh20/RF24
 *
 * and my Nrf24Payload library
 * https://github.com/theapi/nrf24/tree/master/Nrf24Payload
 */

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Nrf24Payload.h>

#define PIN_CE  7
#define PIN_CSN 8

#define PIN_THROTTLE A0
#define PIN_YAW      A1
#define PIN_PITCH    A2
#define PIN_ROLL     A3

#define DEVICE_ID 'T'

RF24 radio(PIN_CE, PIN_CSN);

Nrf24Payload tx_payload = Nrf24Payload();

// Small ack payload for speed
typedef struct{
  uint16_t key;
  uint16_t val;
}
ack_t;
ack_t ack_payload;

//uint16_t msg_id = 0;

byte pipe_tx[6] = TX_ADDRESS;
byte pipe_rx[6] = RX_ADDRESS;

byte counter = 1;

void setup() 
{
  Serial.begin(57600);
  printf_begin();
  Serial.println("Begin");
    
  radio.begin();
  radio.setChannel(80);
  // For the slower data rate (further range) see https://github.com/TMRh20/RF24/issues/98
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(1);
  radio.enableAckPayload();
  
  // How long to wait between each retry, in multiples of 250us,
  // max is 15.  0 means 250us, 15 means 4000us.
  // count How many retries before giving up, max 15
  // 3 * 250 = 750 = time required for an 8 byte ack
  radio.setRetries(3,4);
  radio.setPayloadSize(sizeof(ack_t));
  
  radio.openWritingPipe(pipe_tx);
  radio.openReadingPipe(1, pipe_rx);
  
  // Dump the configuration of the rf unit for debugging
  radio.printDetails();                   
  
  radio.startListening();
 
  tx_payload.setId(0);
  tx_payload.setType('T');
  
  Serial.println("End begin");
}

void loop(void)
{
  tx_payload.setId(tx_payload.getId() + 1);
  tx_payload.setA(tx_payload.getA() + 1);
   
  // Stop listening so we can talk.
  radio.stopListening();                                  
        
  printf("Now sending %d as payload.\n\r ",tx_payload.getId());

  // Take the time, and send it.  This will block until complete 
  unsigned long time = micros();  
  if (!radio.write( &tx_payload, Nrf24Payload_SIZE)) {
    // Got no ack.
    printf("no ack.\n\r"); 
  } else {
     if (!radio.available()) { 
       // blank payload
       printf("Blank Payload Received\n\r"); 
     } else {
       
       while(radio.available() ){
          unsigned long tim = micros();
         
          radio.read( &ack_payload, sizeof(ack_t) );
          printf("Got response %d - %d, round-trip delay: %lu microseconds\n\r", ack_payload.key, ack_payload.val, tim-time);

          counter++;
        }

     }
  }
    
  delay(1000); // Temporary delay
}

