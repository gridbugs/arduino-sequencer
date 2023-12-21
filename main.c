#include "stdint.h"
#include <stdio.h>
#include <avr/io.h>

// The arduino clock is 16Mhz and the USART0 divides this clock rate by 16
#define USART0_CLOCK_HZ 1000000
#define BAUD_RATE_HZ 9600
#define UBRR_VALUE (USART0_CLOCK_HZ / BAUD_RATE_HZ)

// Send a character over USART0.
int USART0_tx(char data, struct __file* _f) {
    while (!(UCSR0A & (1 << UDRE0))); // wait for the data buffer to be empty
    UDR0 = data; // write the character to the data buffer
    return 0;
}

// Create a stream associated with transmitting data over USART0 (this will be
// used for stdout so we can print to a terminal with printf).
static FILE uartout = FDEV_SETUP_STREAM(USART0_tx, NULL, _FDEV_SETUP_WRITE);

void USART0_init( void ) {
    UBRR0H = (UBRR_VALUE >> 8) & 0xF; // set the high byte of the baud rate
    UBRR0L = UBRR_VALUE & 0xFF; // set the low byte of the baud rate
    UCSR0B = 1 << TXEN0; // enable the USART0 transmitter
    UCSR0C = 3 << UCSZ00; // use 8-bit characters
    stdout = &uartout;
}

void ADC_init(void) {
    PRR &= ~(1 << PRADC); // disable power reduction ADC bit
    ADCSRA = (1 << ADEN); // enable the ADC
    DIDR0 = 0xFF; // enable all the ADC pins
}

void ADC_set_channel(uint8_t channel) {
    ADMUX = (1 << REFS0) | channel;
}

void ADC_start_read(void) {
    ADCSRA |= (1 << ADSC); // start the conversion
}

uint16_t ADC_complete_read(void) {
    while ((ADCSRA & (1 << ADSC)) != 0); // wait for the start bit to clear
    uint16_t lo = (uint16_t)ADCL;
    uint16_t hi = (uint16_t)ADCH << 8;;
    return hi | lo;
}

uint16_t ADC_read(uint8_t channel) {
    ADC_set_channel(channel);
    ADC_start_read();
    return ADC_complete_read();
}

uint16_t ADC_read_discarding_first(uint8_t channel) {
    ADC_read(channel);
    return ADC_read(channel);
}

int main(void) {
    USART0_init();
    printf("Hello, World!\r\n");

    DDRB = 0xFF;
    DDRD = 0xFF;

    ADC_init();

    uint8_t index = 0;

    while (1) {
        uint32_t note_duration = ((uint32_t)ADC_read_discarding_first(6)) >> 7;
        uint32_t gate_up_duration = ((uint32_t)ADC_read_discarding_first(7)) >> 7;
        PORTB = index;
        index = (index + 1) % 16;
        uint32_t time = 0;
        PORTB |= (1 << 4);
        while (time < gate_up_duration) {
            time += 1;
        }
        PORTB &= ~(1 << 4);
        while (time < note_duration) {
            time += 1;
        }
    }

    return 0;
}
