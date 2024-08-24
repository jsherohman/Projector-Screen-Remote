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
//#define SCREEN_DOWN 0xECEF // may need reversing for endianness
#define SCREEN_UP 0x7EECF0

uint8_t temp = 0;
// Phase variable stores which part of the waveform generation we are in
// 0x00 = start of new bit, 0x01 = off time for zero value/short, 0x02 = off time for one value/long
uint8_t phase = 0x00;

uint8_t bitsToSend = 24; // Always 24 bits
static uint32_t sendBuf = SCREEN_DOWN;

int main (void)
{
  DDRB = _BV(PB0) | _BV(PB1); //Set PB0 as output, ignore the rest
  PORTB = _BV(0x00); //Start all bits of Port B at 0; we only care about PB0 though

  //TCNT0 = 0; // Set counter 0 to zero

  OCR0A = 25u; // Set compare register A to 255 to start
  TCCR0A = _BV(COM0A0) | _BV(WGM01); // Toggle OC0A on match; CTC mode
  //TCCR0A =  _BV(WGM01); // Toggle OC0A on match; CTC mode
  TCCR0B = _BV(CS01) | _BV(CS00); // clock frequency / 64
  TIMSK0 = _BV(OCIE0A); // Enable compare match A interrupt
  
  sei(); //Enable global interrupts
  
  while(1); // Infinite loop    
}


ISR(TIM0_COMPA_vect) //Timer 0 compare match A interrupt
{
  PORTB |= _BV(PB1); //enable PB1
  /*
  if (!phase) {
      OCR0A = 50u; // Short on sequence for a zero
  } else {
      OCR0A = 100u; // Short on sequence for a zero
  }
  phase ^= 0x01; // Toggle phase
  */
  
  if (phase == 0x00) { // Start of a bit
    if (sendBuf & 0x01) {
      OCR0A = 100u; // Long on sequence for a one
    } else {
      OCR0A = 50u; // Short on sequence for a zero
    }
  } else { // end of a bit
    if (sendBuf & 0x01) {
      OCR0A = 50u; // Short off sequence for a one
    } else {
      OCR0A = 100u; // Long off sequence for a zero
    }

    /*
    if (bitsToSend > 0) {
      sendBuf >>= 1;
      bitsToSend -= 1;
    } else {
      sendBuf = SCREEN_DOWN;
      bitsToSend = 24;
      //sendBuf ^= 0x01;
    }
    */
    
  }
  phase ^= 0x01; // Toggle phase
  //TCNT0 = 0;
  //OCR0A = temp;
  PORTB ^= _BV(PB1); //clear PB1
}