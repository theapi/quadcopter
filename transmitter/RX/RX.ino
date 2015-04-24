
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#define PIN_CE  7
#define PIN_CSN 8

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


void setup() 
{
  //Serial.begin(57600);
  //printf_begin();
  //Serial.println("Begin");
  
  memset(&tx_payload, 0, sizeof(tx_t));
   
  radio.begin();
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
      radio.read( &tx_payload, sizeof(tx_t) );
      ack_ready = 0;

      ack_payload.key = 1;
      ack_payload.val = tx_payload.throttle;  
        
   }

}

