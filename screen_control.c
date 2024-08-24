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
#include <util/delay.h>

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
  PORTB = 0x00; //Start all bits of Port B at 0; we only care about PB0 though

  while(1) {
    sendData(SCREEN_DOWN, 24);
    //writeOne();
    _delay_ms(150);
  }
}

void writeZero() {
  PORTB |= _BV(PB0);
  _delay_ms(3);
  PORTB &= !_BV(PB0);
  _delay_ms(6);
}

void writeOne() {
  PORTB |= _BV(PB0);
  _delay_ms(6);
  PORTB &= !_BV(PB0);
  _delay_ms(3);
}

void sendData(uint32_t buf, uint8_t bitsToSend) {
  for (int i = 0; i < bitsToSend; i++) {
    if (buf & 0x01) {
      writeOne();
    } else {
      writeZero();
    }
    buf >>= 1;
  }
}