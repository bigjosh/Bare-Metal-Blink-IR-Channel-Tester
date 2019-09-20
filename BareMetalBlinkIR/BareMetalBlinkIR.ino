// Used for characterizing the IR communications channel between two blinks

// Talks to blinks hardware directly and bybasses all blinklib and blinkOS stuff, 
// so this will never sleep or accept a download or anythign like that, so be sure
// to program some normal software back onto the blink after you are done with this.

// To use, connect an ossciliscope to service port pin 0 (GND) and 3 (T).
// (Sorry, but the A pin does not seem to work on newer blinks. :( )



#define IR_LED_TO_TEST      4      // 0-5 
#define TX_MODE             0      // 1=trasnmit, 0=receive

#define IR_LED_ON_TIME_US   10

#include "hardware.h"
#include "util/delay.h"   // Don't tell anyone that you can do this since normal blink code never should

#define F_CPU 8000000

void tx_mode() {
  
  // Use our own infinte loop here to keep blinks stuff from running when we return from loop()

  while (1) {

    // Service Port A pin on
    SP_T_PORT |= _BV( SP_T_BIT );

    // IR LED on
    IR_ANODE_PORT |= _BV( IR_LED_TO_TEST );
    
    // LED ON
    _delay_us(IR_LED_ON_TIME_US);
  
    // IR LED off
    IR_ANODE_PORT &= ~_BV( IR_LED_TO_TEST );
  
    // Service Port A pin off
    SP_T_PORT &= ~_BV( SP_T_BIT );
  
    // Wait between flashes
    // Gives time for scope the sync and for rx blink to recharge
    _delay_us( IR_LED_ON_TIME_US*3  );

  }
  
}


void rx_mode() {
  
  // Use our own infinte loop here to keep blinks stuff from running when we return from loop()

  while (1) {

    // First charge the LED
   
    IR_CATHODE_PORT |= _BV( IR_LED_TO_TEST );
    _delay_us(5);
    IR_CATHODE_DDR &= ~_BV( IR_LED_TO_TEST );
    IR_CATHODE_PORT&= ~_BV( IR_LED_TO_TEST );

    // Ok, cathode is now charged and ready to recieve.


    // Service port on
    SP_T_PORT |= _BV( SP_T_BIT );

    // Wait for cathode to go low
    while ( IR_CATHODE_PIN & _BV( IR_LED_TO_TEST ) );

    // Servce port low
    SP_T_PORT &= ~_BV( SP_T_BIT );
    
    // Pause so we do not trigger again on the same incoming pulse
    // 20 percent extra to account for clock differences between TX and RX
    _delay_us( (IR_LED_ON_TIME_US * 120) / 100 );
         
  }
  
}



void setup() {

  // Stop all the normal blinks infrastructure from running
  cli();

  // Set Service Port A pin to output and OFF
  SP_T_PORT &= ~_BV( SP_T_BIT );
  SP_T_DDR |= _BV( SP_T_BIT);  

  // Set IRs off and output
  IR_ANODE_PORT &= ~IR_BITS;
  IR_ANODE_DDR |= IR_BITS;

  IR_CATHODE_PORT &= ~IR_BITS;
  IR_CATHODE_DDR |= IR_BITS;

  if (TX_MODE) {

    tx_mode();
    
  } else {

    rx_mode();
  
  }

}


void loop() {
}
