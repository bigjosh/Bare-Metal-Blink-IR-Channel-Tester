// Used for characterizing the IR communications channel between two blinks

// RED   = RX mode
// GREEN = TX mode
// Button press and release cycles though mode+face

// Talks to blinks hardware directly and bybasses all blinklib and blinkOS stuff, 
// so this will never sleep or accept a download or anythign like that, so be sure
// to program some normal software back onto the blink after you are done with this.

// To use, connect an ossciliscope to service port pin 0 (GND) and 3 (T).
// (NB: the A pin does not seem to work on newer blinks. :( )
// (NNB: The R and T pins seem to be swapped on newer blinks ) 

// On TX blink, signal goes high when IR LED is ON
// On RX blink, signal goes low when enough photons received to trigger


#define IR_LED_ON_TIME_US   25

#include "hardware.h"
#include "util/delay.h"   // Don't tell anyone that you can do this since normal blink code never should

#define F_CPU 8000000


// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

#define BUTTON_DOWN() (!TBI(BUTTON_PIN,BUTTON_BIT))

typedef  volatile uint8_t *io_address;

io_address pixel_ports[] = { &PIXEL0_PORT , &PIXEL1_PORT , &PIXEL2_PORT , &PIXEL3_PORT , &PIXEL4_PORT , &PIXEL5_PORT };
io_address pixel_ddrs[]  = { &PIXEL0_DDR  , &PIXEL1_DDR  , &PIXEL2_DDR  , &PIXEL3_DDR  , &PIXEL4_DDR  , &PIXEL5_DDR };
uint8_t    pixel_bits[]  = { PIXEL0_BIT   , PIXEL1_BIT   , PIXEL2_BIT   , PIXEL3_BIT   , PIXEL4_BIT   , PIXEL5_BIT };


void blink_hard() {

  SBI( PIXEL0_PORT , PIXEL0_BIT );
  SBI( PIXEL0_DDR , PIXEL0_BIT );

  CBI( LED_G_PORT , LED_G_BIT );
  SBI( LED_G_DDR , LED_G_BIT );
  
  
}


// We need to disble the timers so we can directly aaccess the LED pins

void disable_timers() {

      TCCR0B = 0;                     // Timer0 stopped, so no ISR can change anything out from under us
    // Right now one LED has its anode activated so we need to turn that off
    // before driving all cathodes low



    TCCR2B = 0;                     // Timer/counter2 stopped.


    // PWM outputs will be stuck where ever they were, at this point.
    // Lets set them all low so no place for current to leak.
    // If diode was reverse biases, we will have a tiny leakage current.

    TCCR0A = 0;         // Disable both timer0 outputs
    TCCR2A = 0;         // Disable timer2 output

}

// Get pixels ready for direct access 

void clear_pixels() {

    // Deactivate anodes
    CBI( PIXEL0_PORT , PIXEL0_BIT );
    CBI( PIXEL1_PORT , PIXEL1_BIT );
    CBI( PIXEL2_PORT , PIXEL2_BIT );
    CBI( PIXEL3_PORT , PIXEL3_BIT );
    CBI( PIXEL4_PORT , PIXEL4_BIT );
    CBI( PIXEL5_PORT , PIXEL5_BIT );  

    disable_timers();

}

void blink(  io_address cport, io_address cddr, int cbit , int face  ) {
  CBI( *cport , cbit );  // Cathode low
  SBI( *cddr , cbit );   // cathode out

  SBI( *pixel_ports[face] , pixel_bits[face] );  // Anode high
  SBI( *pixel_ddrs[face]  , pixel_bits[face] );  // Anode out

  _delay_us(1);

  CBI( *pixel_ports[face] , pixel_bits[face] );  // Anode high
                                                // (Leave anode out)
  


  SBI( *cport , cbit );  // Cathode high
                         // (leave cathode out)
    
}

void blink_green( uint8_t face ) {

  blink(  &LED_G_PORT , &LED_G_DDR , LED_G_BIT , face);
  
}

void blink_red( uint8_t face ) {

  blink(  &LED_R_PORT , &LED_R_DDR , LED_R_BIT ,  face);
   
}

template <int FACE>
void tx_mode() {
  
  // Use our own infinte loop here to keep blinks stuff from running when we return from loop()

  while (!BUTTON_DOWN()) {   

    // Service Port A pin on
    SP_T_PORT |= _BV( SP_T_BIT );

    // IR LED on
    IR_ANODE_PORT |= _BV( FACE );
    
    // LED ON
    _delay_us(IR_LED_ON_TIME_US);
  
    // IR LED off
    IR_ANODE_PORT &= ~_BV( FACE );
  
    // Service Port A pin off
    SP_T_PORT &= ~_BV( SP_T_BIT );
  
    // Wait between flashes
    // Gives time for scope the sync and for rx blink to recharge
    _delay_us( IR_LED_ON_TIME_US*3  );

    blink_green(FACE );

  }
  
}

template <int FACE>
void rx_mode() {
  
  // Use our own infinte loop here to keep blinks stuff from running when we return from loop()

  while (!BUTTON_DOWN()) {

    // First charge the LED
   
    IR_CATHODE_PORT |= _BV( FACE );     // Cathode high (reverse chanrge) (in default OUTPUT state)
    _delay_us(5);
    IR_CATHODE_DDR &= ~_BV( FACE );     // Cathode input mode
    IR_CATHODE_PORT&= ~_BV( FACE );     // Cathode pull-up off
                                        // (order matters here, if we set PORT low first, then we would drain the charge)

    // Ok, cathode is now charged and ready to recieve.

    // Service port on
    SP_T_PORT |= _BV( SP_T_BIT );

    // Wait for cathode to go low
    while ( IR_CATHODE_PIN & _BV( FACE ) );

    // Servce port low
    SP_T_PORT &= ~_BV( SP_T_BIT );

    IR_CATHODE_DDR |= ~_BV( FACE );     // Cathode back to default OUTPUT state 
    
    // Pause so we do not trigger again on the same incoming pulse
    // 20 percent extra to account for clock differences between TX and RX
    _delay_us( (IR_LED_ON_TIME_US * 120) / 100 );

    blink_red(FACE );
         
  }
  
}

// Sorry for these. I tried to do with with metaprogramming, but it did not work.
// I think the recusive calls were blowing the stack. 

void rx_mode_dispatch( int face ) {

  switch (face) {

    case 0: rx_mode<0>(); break;
    case 1: rx_mode<1>(); break;
    case 2: rx_mode<2>(); break;
    case 3: rx_mode<3>(); break;
    case 4: rx_mode<4>(); break;
    case 5: rx_mode<5>(); break;
  
  }
  
}

void tx_mode_dispatch( int face ) {

  switch (face) {

    case 0: tx_mode<0>(); break;
    case 1: tx_mode<1>(); break;
    case 2: tx_mode<2>(); break;
    case 3: tx_mode<3>(); break;
    case 4: tx_mode<4>(); break;
    case 5: tx_mode<5>(); break;
  
  }
  
}



void waitfor_button_up() {

    _delay_ms(50);      // Debounce
  
    while (BUTTON_DOWN());
    
    _delay_ms(50);      // Debounce

}

void setup() {

  // Stop all the normal blinks infrastructure from running
  cli();

  clear_pixels();


  // Set Service Port A pin to output and OFF
  SP_T_PORT &= ~_BV( SP_T_BIT );
  SP_T_DDR |= _BV( SP_T_BIT);  

  // Set IRs off and output
  IR_ANODE_PORT &= ~IR_BITS;
  IR_ANODE_DDR |= IR_BITS;

  IR_CATHODE_PORT &= ~IR_BITS;
  IR_CATHODE_DDR |= IR_BITS;

  while (1) {

    FOREACH_FACE(f) {
  
      tx_mode_dispatch(f);    
  
      waitfor_button_up();
  
      rx_mode_dispatch(f);    
  
      waitfor_button_up();
      
    }

  }

}


void loop() {
}
