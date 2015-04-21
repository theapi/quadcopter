

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Nrf24Payload.h>

#define PIN_CE  7
#define PIN_CSN 8

RF24 radio(PIN_CE, PIN_CSN);

Nrf24Payload rx_payload = Nrf24Payload();

// Small (4 bytes) ack payload for speed
typedef struct{
  uint16_t key;
  uint16_t val;
}
ack_t;
ack_t ack_payload;

uint16_t msg_id = 0;

byte pipe_tx[6] = TX_ADDRESS;
byte pipe_rx[6] = RX_ADDRESS;


void setup() 
{
  //Serial.begin(57600);
  //printf_begin();
  //Serial.println("Begin");
   
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

  radio.openWritingPipe(pipe_rx);        
  radio.openReadingPipe(1, pipe_tx);
  
  radio.startListening();
  //radio.printDetails();
  
  //Serial.println("End begin");
}


void loop(void) 
{
    static byte ack_ready = 0;
    
    if (!ack_ready) {
      // Create the ack payload read for the next transmission response
      radio.writeAckPayload(1, &ack_payload, sizeof(ack_t) ); 
      ack_ready = 1;
    }
    
    if ( radio.available()) {  
      radio.read( &rx_payload, Nrf24Payload_SIZE );
      ack_ready = 0;

      ack_payload.key = 1;
      ack_payload.val = rx_payload.getId();  
        
   }

}

