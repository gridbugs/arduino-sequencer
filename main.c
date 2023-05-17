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

// Represents a single I/O-Port pin
typedef struct {
    volatile uint8_t* port; // pointer to the port's register
    uint8_t bit; // (0 to 7) which bit in the register the port corresponds to
} led_t;

// List out each pin with an attached LED in the order we want them to flash
led_t leds[] = {
    { .port = &PORTD, .bit = 7 },
    { .port = &PORTD, .bit = 6 },
    { .port = &PORTD, .bit = 5 },
    { .port = &PORTD, .bit = 4 },
    { .port = &PORTD, .bit = 3 },
    { .port = &PORTD, .bit = 2 },
    { .port = &PORTB, .bit = 2 },
    { .port = &PORTB, .bit = 1 },
    { .port = &PORTB, .bit = 0 },
    { .port = &PORTC, .bit = 5 },
    { .port = &PORTC, .bit = 4 },
    { .port = &PORTC, .bit = 3 },
    { .port = &PORTC, .bit = 2 },
    { .port = &PORTC, .bit = 1 },
    { .port = &PORTC, .bit = 0 },
};

// The number of LEDs
#define N_LEDS (sizeof(leds) / sizeof(led_t))

// Turn on a single LED without affecting the state of the other LEDs
void led_on(led_t led) {
    *led.port |= 1 << led.bit;
}

int main(void) {
    USART0_init();
    printf("Hello, World!\r\n");

    // Set the data direction for each I/O-Port pin to "output".
    // Each DDRx register controls whether each pin in I/O-Port x is
    // an input pin (0) or an output pin (1).
    DDRB = 0xFF;
    DDRC = 0xFF;
    DDRD = 0xFF;

    // The starting points for the indices into the global `leds` array
    // that will be on. We'll have 3 lights on at a time in a rotating
    // pattern, evenly spaced out.
    int indices[] = {0, 5, 10};

    while (1) {

        // Briefly turn off all the LEDs
        PORTB = 0;
        PORTC = 0;
        PORTD = 0;

        // Turn on just the pins at the indices, and increment each index
        // wrapping around at N_LEDS
        for (int i = 0; i < (sizeof(indices) / sizeof(indices[0])); i++) {
            led_on(leds[indices[i]]);
            indices[i] = (indices[i] + 1) % N_LEDS;
        }

        // Wait for a short amount of time before progressing
        uint32_t delay = 100000;
        while (delay > 0) {
            delay -= 1;
        }
    }

    return 0;
}
