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
    ADMUX = 0; // use AREF pin for reference voltage, right adjust the result, select ADC0 channel
    ADCSRA = (1 << ADEN); // enable the ADC
    DIDR0 = (1 << 7); // XXX this just enables the 7th adc channel
}

void ADC_start_read(uint8_t channel) {
    ADMUX &= ~0xF; // clear the channel bits
    ADMUX |= (channel & 0xF); // set the channel
    ADCSRA |= (1 << ADSC); // start the conversion
}

uint16_t ADC_complete_read(void) {
    while ((ADCSRA & (1 << ADSC)) != 0); // wait for the start bit to clear
    uint16_t lo = (uint16_t)ADCL;
    uint16_t hi = (uint16_t)ADCH << 8;;
    return hi | lo;
}

uint16_t ADC_read(uint8_t channel) {
    ADC_start_read(channel);
    return ADC_complete_read();
}

// Represents a single I/O-Port pin
typedef struct {
    volatile uint8_t* port; // pointer to the port's register
    uint8_t bit; // (0 to 7) which bit in the register the port corresponds to
} led_t;

// List out each pin with an attached LED in the order we want them to flash
led_t leds[] = {
    { .port = &PORTD, .bit = 2 },
    { .port = &PORTD, .bit = 3 },
    { .port = &PORTD, .bit = 4 },
    { .port = &PORTD, .bit = 5 },
    { .port = &PORTD, .bit = 6 },
    { .port = &PORTD, .bit = 7 },
    { .port = &PORTB, .bit = 0 },
    { .port = &PORTB, .bit = 1 },
    { .port = &PORTB, .bit = 2 },
    { .port = &PORTB, .bit = 3 },
    { .port = &PORTB, .bit = 4 },
    { .port = &PORTB, .bit = 5 },
    { .port = &PORTC, .bit = 0 },
    { .port = &PORTC, .bit = 1 },
    { .port = &PORTC, .bit = 2 },
    { .port = &PORTC, .bit = 3 },
};

// The number of LEDs
#define N_LEDS (sizeof(leds) / sizeof(led_t))

// Turn on a single LED without affecting the state of the other LEDs
void led_on(led_t led) {
    *led.port |= 1 << led.bit;
}

void led_off(led_t led) {
    *led.port &= ~(1 << led.bit);
}

led_t short_mode_status_led = { .port = &PORTD, .bit = 0 };
led_t freeze_status_led = { .port = &PORTD, .bit = 1 };

int read_tempo(void) {
    return (1023 - ADC_read(7)) / (1 << 1);
}

int main(void) {
    ADC_init();
    //USART0_init();
    //printf("Hello, World!\r\n");

    // Set the data direction for each I/O-Port pin to "output".
    // Each DDRx register controls whether each pin in I/O-Port x is
    // an input pin (0) or an output pin (1).
    DDRB = 0xFF;
    DDRC = (0xFF & ~((1 << 4) | (1 << 5)));;
    DDRD = 0xFF;

    int index = 0;

    int short_mode = 0;
    int tempo = read_tempo();
    int freeze = 0;
    int freeze_toggle_state = 0;
    int short_mode_toggle_state = 0;

    while (1) {

        // read every second frame because some wires cause the adc value to spike?
        if (index % 2 == 0) {
           tempo = read_tempo();
        }

        // Briefly turn off all the LEDs
        PORTB = 0;
        PORTC = 0;
        PORTD = 0;


        led_on(leds[index]);

        if (freeze) {
            led_on(freeze_status_led);
        } else {
            index = (index + 1) % N_LEDS;
        }

        if (short_mode) {
            led_on(short_mode_status_led);
            if (index == 8) {
                index = 0;
            }
        }


        uint32_t delay = ((uint32_t)tempo * (1 << 9));
        while (delay > 0) {
            int freeze_toggle = PINC & (1 << 4);
            if (!freeze_toggle_state && freeze_toggle) {
                freeze = !freeze;
            }
            freeze_toggle_state = freeze_toggle;
            int short_mode_toggle = PINC & (1 << 5);
            if (!short_mode_toggle_state && short_mode_toggle) {
                short_mode = !short_mode;
                if (short_mode) {
                    led_on(short_mode_status_led);
                } else {
                    led_off(short_mode_status_led);
                }
            }
            short_mode_toggle_state = short_mode_toggle;
            delay -= 1;
        }
        /*
        PORTB = 0;
        PORTC = 0;
        PORTD = 0;

        uint32_t delay2 = 100000;
        while (delay2 > 0) {
            delay2 -= 1;
        }*/
    }

    return 0;
}
