

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Nrf24Payload.h>

#define PIN_CE  7
#define PIN_CSN 8

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
  radio.setAutoAck(1);
  radio.enableAckPayload();
  radio.setRetries(0,4);
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

