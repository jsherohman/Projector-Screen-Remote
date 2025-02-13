#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

//#define SCREEN_DOWN 0x007EECE8 // may need reversing for endianness
#define SCREEN_DOWN 0x002E6EFC
//#define SCREEN_UP   0x007EECF0 // may need reversing for endianness
#define SCREEN_UP   0x001E6EFC
#define CMD_LENGTH  25
#define REPETITIONS 10

#define ON_SHORT  52u // 340 usec
#define ON_LONG   152u // 980 usec
#define OFF_SHORT 52u // 340 usec
#define OFF_LONG  152u // 980 usec

#define PHASE_ON    0x00
#define PHASE_OFF   0x01
#define PHASE_DELAY 0x02

uint8_t phase = PHASE_ON;
uint8_t lowering = 0x01;
uint8_t bitsToSend = CMD_LENGTH;
uint8_t repetitions = REPETITIONS;
static uint32_t sendBuf = SCREEN_DOWN;

int main (void)
{
  DDRB = _BV(PB0) | _BV(PB2); // PB0 is radio control pin, PB2 is for monitoring interrupts (PB1 is comparator input)
  PORTB = 0x00; // Start all bits of Port B at 0

  ACSR = _BV(ACBG) | _BV(ACIS1) | _BV(ACIS0); // Use internal ~1.1V ref for comparator + input
  DIDR0 = _BV(AIN1D); // Disable digital input on AIN1 since pin is being used for analog comparator

  CLKPR = _BV(CLKPCE);
  CLKPR = 0x00; // No prescaler

  _delay_ms(200);
  sendPreamble();

  TCNT0 = 0; // Reset timer
  OCR0A = 20u; // Set compare register A to 20 to start
  TCCR0A = _BV(COM0A0) | _BV(WGM01); // Toggle OC0A on match; CTC mode
  TCCR0B = _BV(CS01) | _BV(CS00); // clock frequency / 64
  TIMSK0 = _BV(OCIE0A); // Enable compare match A interrupt
  
  sei(); // Enable global interrupts
  
  while(1); // Infinite loop
}

void sendPreamble() {
  PORTB |= _BV(PB0);
  _delay_us(2100);
  PORTB &= ~_BV(PB0);
  _delay_us(1060);
  PORTB |= _BV(PB0);
  _delay_us(360);
  PORTB &= ~_BV(PB0);
  _delay_ms(11);
}

ISR(TIM0_COMPA_vect) // Timer 0 compare match A interrupt - generate waveforms
{
  PORTB |= _BV(PB2); // enable PB1
  
  switch (phase) {
    case PHASE_ON: // Set on time for a zero or one
      // Long on sequence for a one, short for a zero
      OCR0A = (sendBuf & 0x01) ? ON_LONG : ON_SHORT;
      phase = PHASE_OFF;
      break;
    case PHASE_OFF: // Set off time for a zero or one and shift buffer or switch to idle
      bitsToSend -= 1;
      if (bitsToSend > 0) {
        // Short off sequence for a one, long for a zero
        OCR0A = (sendBuf & 0x01) ? OFF_SHORT : OFF_LONG;
        sendBuf >>= 1;
        phase = PHASE_ON;
      } else {
        // End of a pattern; reset buffer and switch to idling
        sendBuf = lowering ? SCREEN_DOWN : SCREEN_UP;
        bitsToSend = CMD_LENGTH;
        TCCR0A &= ~(_BV(COM0A0) | _BV(COM0A1)); // Disable toggle mode to idle output
        PORTB &= ~_BV(PB0); // Ensure state is low
        OCR0A = 200u;
        phase = PHASE_DELAY;
      }
      break;
    case PHASE_DELAY:
      bitsToSend -= 1;
      if (bitsToSend == 0) {
        repetitions -= 1;
        bitsToSend = CMD_LENGTH;
        phase = PHASE_ON;
        if (repetitions > 0) {
          // Switch back to toggle mode so we keep sending packets
          TCCR0A |= _BV(COM0A0);
        } else {
          // Done repeating; prepare for sending a screen_up on power loss
          lowering = 0x00;
          repetitions = 20; // As many as we can
          sendBuf = SCREEN_UP; // Preload the screen up command
          TIMSK0 &= ~_BV(OCIE0A); // Disable compare match A interrupt
          // Enable comparator interrupt
          ACSR |= _BV(ACIE);
        }
      }
      break;
  }
  PORTB &= ~_BV(PB2); // clear PB1
}

ISR(ANA_COMP_vect) // Analog comparator interrupt vector - here if we've lost power
{
  ACSR &= ~_BV(ACIE); // Disable ACR interrupt so we only get tnterrupted for timer A
  sendPreamble();
  TCNT0 = 0; // Reset timer
  OCR0A = 20u; // Set compare register A to 20 to start
  TCCR0A |= _BV(COM0A0); // Enable toggle mode
  TIMSK0 |= _BV(OCIE0A); // Enable compare match A interrupt
  ACSR |= _BV(ACI); // Force clear the interrupt
}