/************************************************************* 
 This sketch makes the pin PB2 (digital pin 2) toggle every
 second (internal oscillator running at 9.6 MHz). It uses Timer0
 or Timer0B, and divide the clock frequncy by 1024.
 The divided frequencies period is multiplied with the
 number of counts Timer0/Timer0B can take before it overflows.
 The number is then multiplied by 37, and gives approximately
 1 second.  

 9.6MHz / 1024 = 9370 Hz        We divide the 9.6 MHz clock by 1024
 1/9370 = 0.0001067s            Period time
 256 * 0.0001067 = 0.027315    
 0.027315 * 37 = 1.01065 ≈ 1s   
 X = 1.01065 / 0.027315 where X is approximately 37
**************************************************************/ 


#include <avr/interrupt.h>
#include <avr/io.h>

#define SCREEN_DOWN 0x7EECE8 // may need reversing for endianness
#define SCREEN_UP   0x7EECF0
#define CMD_LENGTH  24

#define PHASE_ON    0x00
#define PHASE_OFF   0x01
#define PHASE_DELAY 0x02

uint8_t phase = PHASE_ON;

uint8_t bitsToSend = CMD_LENGTH;
static uint32_t sendBuf = SCREEN_DOWN;

int main (void)
{
  DDRB = _BV(PB0) | _BV(PB1); //Set PB0 as output, ignore the rest
  PORTB = _BV(0x00); //Start all bits of Port B at 0; we only care about PB0 though

  OCR0A = 25u; // Set compare register A to 255 to start
  TCCR0A = _BV(COM0A0) | _BV(WGM01); // Toggle OC0A on match; CTC mode
  TCCR0B = _BV(CS01) | _BV(CS00); // clock frequency / 64
  TIMSK0 = _BV(OCIE0A); // Enable compare match A interrupt
  
  sei(); // Enable global interrupts
  
  while(1); // Infinite loop    
}


ISR(TIM0_COMPA_vect) //Timer 0 compare match A interrupt
{
  PORTB |= _BV(PB1); //enable PB1
  
  switch (phase)   {
    case PHASE_ON: // Set on time for a zero or one
      // Long on sequence for a one, short for a zero
      OCR0A = (sendBuf & 0x01) ? 100u : 50u;
      phase = 0x01;
      break;
    case PHASE_OFF: // Set off time for a zero or one and shift buffer or switch to idle
      // Short off sequence for a one, long for a zero
      if (bitsToSend > 0) {
        OCR0A = (sendBuf & 0x01) ? 50u : 100u;
        sendBuf >>= 1;
        bitsToSend -= 1;
        phase = 0x00;
      } else {
        // End of a pattern; reset buffer and switch to idling
        sendBuf = SCREEN_DOWN;
        bitsToSend = CMD_LENGTH;
        TCCR0A &= ~(_BV(COM0A0) | _BV(COM0A1)); // Disable toggle mode to idle output
        PORTB &= ~_BV(PB0); //Ensure state is low
        OCR0A = 150u;
        phase = PHASE_DELAY;
      }
      break;
    case PHASE_DELAY: 
      bitsToSend -= 1;
      if (bitsToSend == 0) {
        phase = PHASE_ON;
        bitsToSend = CMD_LENGTH;
        TCCR0A |= _BV(COM0A0);
      }
  }
  PORTB &= ~_BV(PB1); //clear PB1
}