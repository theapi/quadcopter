/**
 * Testing pwm frequencies while the nrf24 is using SPI on 11
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define PIN_THROTTLE A0    // input pin of the potentiometer
#define PIN_MOTOR    5     // pwm on timer0 - the timer that does micros(), millis() etc

#define RX_ADDRESS "001RX"
#define TX_ADDRESS "001TX"

#define PIN_CE  7
#define PIN_CSN 8

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
ack_t ack_payload;

byte pipe_tx[6] = TX_ADDRESS;
byte pipe_rx[6] = RX_ADDRESS;

int vcc = 3800;

void setup() {
  Serial.begin(57600);
  printf_begin();
  Serial.println("Begin");
  
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
  radio.printDetails();
}

void loop() {
  
    static byte ack_ready = 0;
    static byte ack_key = 0;
    
    if (!ack_ready) {
      // Create the ack payload read for the next transmission response
      radio.writeAckPayload(1, &ack_payload, sizeof(ack_t) ); 
      ack_ready = 1;
    }
    
    if ( radio.available()) {  
      radio.read( &tx_payload, sizeof(tx_t) );
      ack_ready = 0;

      ack_payload.key = ack_key;
      switch (ack_key) {
        case 0:
          ack_payload.val = vcc; 
          break;
        case 1:
          ack_payload.val = tx_payload.throttle; 
          break;
        case 2:
          ack_payload.val = tx_payload.yaw; 
          break;
        case 3:
          ack_payload.val = tx_payload.pitch; 
          break;
        case 4:
          ack_payload.val = tx_payload.roll; 
          break;
      }  
        
      if (++ack_key > 4) {
        ack_key = 0; 
      }
   }
 
 
  // Prove the main timer is working
  unsigned long now = millis();
  //Serial.println(now);
  
  //byte pwm = analogRead(PIN_THROTTLE) / 4;
  
  
  byte pwm = (tx_payload.yaw -1000) / 4;
  if (pwm < 15) {
    // Uncalibrated sticks so just oof at low enough value
    pwm = 0; 
  }
  
  analogWrite(PIN_MOTOR, pwm);   

  //Serial.println();  
  //delay(100);
}

