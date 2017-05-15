#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define USART_BAUDRATE 19200
#define BAUD_PRESCALER ( (F_CPU / (USART_BAUDRATE * 16UL)) - 1)
#define KB_ENTER		0x0D

char str[100];
int numbers [6];


/**
 * @brief Send a single byte.
 * @param[in] byte  Byte to send
 */
void usart_send_byte(uint8_t byte) {
	/* Wait for empty transmit buffer */
	while ( !(UCSR0A & (1 << UDRE0)) ) {
	}

	/* Send byte */
	UDR0 = byte;
}


/**
 * @brief Configure the USART0 port.
 */
void serial_init(void)
{
	/* Set baud rate */
	//uint16_t baud = BAUD_PRESCALER;
	UBRR0H = (uint8_t)0;
	UBRR0L = (uint8_t)51;

	/* Enable received and transmitter */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	/* Set frame format (8N1) */
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
	UCSR0C &= ~(1 << UMSEL00);
	UCSR0B |= (1 << RXCIE0);
	//sei();
}

/**
 * @brief Transmit a string.
 * @param[in] s  Null terminated string to send
 */
void usart_print(char *s)
{
	//PORTB ^= (1 << PORTB5);

	while (*s != 0) {
		usart_send_byte( *(s++) );

		/* Let the VM breath */
		_delay_ms(1);
	}
}

/**
 * @brief Check for incomming data.
 * @return 1 If there is data avaliable, 0 otherwise
 */
uint8_t usart_data_available(void)
{
	if ( UCSR0A & (1 << RXC0) )
		return 1;
	return 0;
}

/**
 * @brief Get incoming data.
 * @return Received byte.
 */
uint8_t usart_recv_byte(void)
{
	/* Wait until data is available */
	while ( !usart_data_available() ){
	}

	/* Read byte */
	return UDR0;
}

char prbuff[120];
void serial_printf(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vsnprintf(prbuff, 120, format, args);
    usart_print(prbuff);
    va_end(args);
}

static uint8_t append_char(char *str, uint8_t ch, uint8_t k, uint8_t max) {
	usart_send_byte(ch);

    if (k < max - 1)
        str[k++] = ch;
    return k;
}

uint8_t usart_read_str(char *str, uint8_t max) {
    uint8_t k, eos, ch;

    k = 0;          // index of first free position in the string
    eos = 0;        // end of string, used as boolean

    while (!eos) {
        ch = usart_recv_byte();
        if (ch == '\r') {
                eos = 1;
        }
        else {
            k = append_char(str, ch, k, max);
        }
    }

    // terminate string accordingly
    str[k] = '\0';

    /* return input length */
    return k;
}

void toggle_led(void)
{
	PORTB ^= (1 << PORTB5);
}

int monitorPins(int timeout) {
	int i;
	int res;
	int pin = -1;
	int pinTriggered = 0;
	int diff = 0;
	while (diff <= timeout && pinTriggered == 0){
		res = (PINB<<8)+PIND;
		if (res > 3) {
			//serial_printf("Result: %d\r\n", res);
			for (i=0;i<6;i++) {
				int bit = (res >> (numbers[i])) & 1;
				if (bit==1 && pinTriggered == 0) {
					pin = numbers[i];
					pinTriggered = 1;
				}
				else if (bit == 1 && pinTriggered == 1) {
					pinTriggered = 2;
					pin = -1;
				}
			}

		}
		diff += 20;
		_delay_ms(20);
	}
	return pin;
}

int whack_it() {
	int success;
	int blink;
	unsigned int rnd;
	int timeout;
	for (success=0; success<51; success++)  {


		if (success == 0) {
			serial_printf("\r\nReady?\r\n");
			_delay_ms(1000);
			serial_printf("\r\nGet set!\r\n");
			_delay_ms(1000);
			serial_printf("\r\nGO!\r\n");
			timeout = 5000;
		}
		else {
			timeout = timeout / 2;
		}

		blink = 1;
		rnd = rand() % 6 + 1;

		while (blink <= rnd) {
			toggle_led();
			_delay_ms(50);
			toggle_led();
			_delay_ms(50);
			blink++;
		}
		_delay_ms(4);
		int triggered = monitorPins(timeout);

		if (triggered == numbers[rnd-1]) {
			if (success == 50) {
				return 0xCAFE;
			}
			sprintf(str, "Great job. You whacked it. Only %d more to go.\r\n", 50 - success);
			usart_print(str);
			_delay_ms(50);

		}
		else {
			usart_print("You missed it. Try again by pressing <Enter>.\r\n");
			while (usart_recv_byte() != KB_ENTER) {}
			success = -1;
		}

	}
	return 0;
}

int pins() {
	int res = 0;
	int i=0;
	for (i=0;i<6;i++) {
		numbers[i] = -1;
	}
	i = 0;
	int tmp, j;
	//usart_print("Pin layout:\r\n");
	while (numbers[5] == -1) {
		tmp = rand()%11+2;
		for (j=0; j<6;j++) {
			if (tmp == numbers[j]) {
				break;
			}
			else if (j==5) {
				numbers[i] = tmp;
				i++;
				res = 0xBABE;
			}
		}
	}
	return res;
}

int random_setup() {
    static uint8_t initialized=0;
    if (initialized == 0) {
        srand(eeprom_read_word(0));
        eeprom_write_word(0, rand() ^ 0xBEEFBEFF);
        initialized=1;
        return 0xDEAD;
    }
    return 0;
}


int main(void) {

    serial_init();

    //set LED as output
    DDRB = 0x20;

    //return value checking code.
    int res;

    serial_printf("\r\nWelcome adventurer.\r\n\r\n");
    serial_printf("We are glad you are here. We are in dire need of assistence.\r\n");
    serial_printf("A huge family of moles have found their way into our yard.\r\n");
    serial_printf("We need you to get rid of all 20 of them.\r\n");
    serial_printf("If you manage to extinguish them all we will greatly reward you.\r\n");
    serial_printf("When you are ready, please step into the yard by pressing <Enter>\r\n");

	while (usart_recv_byte() != KB_ENTER) {}

    res = random_setup();
    if (res != 0xDEAD) {
    	return 0;
    }
    res = pins();
    if (res != 0xBABE) {
    	return 0;
    }

    //double check here, just in case we glitch this one.
    //also, random delay in there, because why not.
	res = whack_it();
	if (res == 0xCAFE) {
		int i = 0;
		int rnd = rand()%500;
		while (i < rnd ) {
			i++;;
			_delay_ms(1);
		}
		if (~res == 0x3501) {
			serial_printf("\r\nWhat? You managed to get them all already?\r\n");
			serial_printf("We are most gratefull for your service.\r\n");
			serial_printf("Please take our most precious belonging.\r\n");

            serial_printf("FLAG:S4v3d_the_f4rm!");
            
			return 0;
		}
	}

	usart_print("Failed.\r\n");

    return 0;
}
