
/**
 * RHme2
 * Other - Emergency Transmitter
 *
 */

#define F_CPU 16000000UL
#define USART_BAUDRATE 115200
#define BAUD_PRESCALER ( (F_CPU / (USART_BAUDRATE * 16UL)) - 1)
#define INBUFFER_LEN 16
#define CHR_LF 10

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/power.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "aes.h"
#include <stdio.h>


/************************/
/* Functions prototypes */
/************************/

void usart_send_byte(uint8_t byte);
void serial_init(void);
void usart_print(char *s);
void help(void);
void toggle_led(void);
void execute(void);
void login(void);
void encrypt(void);
void morse_print(uint8_t data);
void morse_flash(uint8_t data);
void aes_setup(void);

uint8_t usart_recv_byte(void);
uint8_t priv_chk(void);


/********************/
/* Global variables */
/********************/

aes_key_t key;

/* A complete command was received */
uint8_t parse_flag;

/* RX buffer */
uint8_t inbuffer[INBUFFER_LEN];

/* Current position inside INBUFFER_LEN */
uint8_t pos_inbuffer;

/* User admin status */
uint8_t admin;

/* Morse code table */
uint8_t mrs_table[16][10] = {"- - - - -",  // 0x30 0
			     ". - - - -",  // 0x31 1
			     ". . - - -",  // 0x32 2
			     ". . . - -",  // 0x33 3
			     ". . . . -",  // 0x34 4
			     ". . . . .",  // 0x35 5
			     "- . . . .",  // 0x36 6
			     "- - . . .",  // 0x37 7
			     "- - - . .",  // 0x38 8
			     "- - - - .",  // 0x39 9
			     ". -\x00*****",  // 0x41 A
			     "- . . .\x00*",  // 0x42 B
			     "- . - .\x00*",  // 0x43 C
			     "- . .\x00***",  // 0x44 D
			     ".\x00*******",  // 0x45 E
			     ". . - .\x00*",  // 0x46 F
			    };

/************************/
/* Function Definitions */
/************************/

int main(void)
{
	uint32_t i;

	aes_setup();

	/* Set the clock to 2MHz to give more chances to properly inject the fault */
	clock_prescale_set(clock_div_8);

	/* Set the LED as output in the Data Direction Register */
	DDRB = 0x20;

	/* Initialize global variables. */
	parse_flag = 0;
	pos_inbuffer = 0;
	admin = 0;

	serial_init();
	help();
	usart_print("\r\n>> ");

	/* Clean inbuffer so all the encryptions are deterministic */
	for (i = 0; i < INBUFFER_LEN; i++) {
		inbuffer[i] = 0;
	}
	/* Wait for commands */
	while (1) {
		if (parse_flag == 1) {
			execute();
			usart_print("\r\n>> ");
		}
	}
	free(key);
	return (0);
}
/**
 * @brief aes_setup Set the key from the flag value
 */
void aes_setup(void)
{
	uint8_t aux[32] = {0xa9, 0xea, 0x57, 0xa7, 0xec, 0xfd, 0x4d, 0x2f, 0x55, 0x6c, 0x81, 0x87, 0x99, 0x3d, 0x7b, 0x29};
	int i;

	key = (uint8_t*)malloc(AES_KEY_SIZE);

	for (i = 0; i < AES_KEY_SIZE; i++) {
		key[i] = aux[i];
	}
	memset(aux, 0, sizeof(aux));
}

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
	UBRR0H = 0; //(uint8_t)(baud >> 8);
	UBRR0L = 12;//(uint8_t) baud;

	UCSR0A |= (1 << U2X0);
	/* Enable received and transmitter */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	/* Set frame format (8N1) */
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
	UCSR0C &= ~(1 << UMSEL00);
	UCSR0B |= (1 << RXCIE0);
	sei();
}

/**
 * @brief Transmit a string.
 * @param[in] s  Null terminated string to send
 */
void usart_print(char *s)
{
	while (*s != 0) {
		usart_send_byte( *(s++) );

		/* Let the VM breathe */
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

/**
 * @brief Display the help menu.
 */
void help(void)
{
	usart_print("                            \r\n");
	usart_print("====== Jungle Assistance System V1.0 ======\r\n\r\n");
	usart_print("This board will help you get out of the jungle in no time!\r\n");
	usart_print("Write a message of maximum 16 bytes asking for help, the message\r\n");
	usart_print("will be transmitted _encrypted_ using the LED and a secret key.\r\n");
	usart_print("The key will remain secure even if the JAS falls into enemy\r\n\r");
	usart_print("hands (We hope so).\r\n\r\n");
	usart_print("As the LED is not powerful enough please aim carefuly.\r\n");

}

/**
 * @brief UART receive interruption.
 *
 * Adds the received characters to te inBuffer and signals
 * when a LF character is received so the command can be
 * parsed.
 */
ISR(USART_RX_vect)
{
	uint8_t data;
	data = UDR0;

	/* CHR_LF signals end of command */
	if (data == CHR_LF) {
		pos_inbuffer = 0;
		parse_flag = 1;

	} else if (pos_inbuffer < INBUFFER_LEN) {
		inbuffer[pos_inbuffer] = data;
		pos_inbuffer++;
	}
}

/**
 * @brief Toggle the led.
 */
void toggle_led(void)
{
	PORTB ^= (1 << PORTB5);
}

/**
 * @brief execute the received command
 */
void execute(void)
{
	encrypt();
	parse_flag = 0;
}

/**
 * @brief Encrypt the message.
 */
void encrypt(void)
{
	aes_state_t data = (aes_state_t) inbuffer;
	uint32_t i;

	aes_key_expansion(key);
	aes_ecb_encrypt(data, key);

	/* Print morse representation on the screen. */
	//    for (i = 0; i < 16; i++) {
	//        morse_print(data[i]);
	//    }
	cli();
	for (i = 0; i < 16; i++) {
		morse_flash(data[i]);
	}
	sei();
	/* If this is enabled, the data can be corrupted everywhere
	* and it becomes TOO dificult to check if the fault is too
	* late.
	*/
	//    for (i = 0; i < 16; i++) {
	//         data[i] = 0;
	//    }
}

/**
 * @brief morse_print Send the ascii dash-dot representation on the UART
 * @param data  Data do send in binary
 *
 * If data is 0xFA, then the morse code for F is send, and then the
 * morse code for A.
 */
void morse_print(uint8_t data)
{
	usart_print( (char *) mrs_table[ (data >> 4) & 0x0F ] );
	usart_print("   ");
	usart_print( (char *) mrs_table[ data & 0x0F ]);
	usart_print("   ");
}

/**
 * @brief morse_flash Flashes the led with a morse pattern.
 * @param data  Data to send in binary
 */
void morse_flash(uint8_t data)
{
	uint8_t* morse_str;
	uint8_t signal;
	uint32_t e;
	const uint32_t dot_time = 100;

	/* Send the most significant nibble */
	morse_str = mrs_table[ (data >> 4) & 0x0F ];
	while (*morse_str) {
		signal = *morse_str;
		switch (signal) {
		case '-':
			PORTB ^= (1 << PORTB5);
			for (e=dot_time * 3; e>0; e--);
			PORTB ^= (1 << PORTB5);
			break;

		case '.':
			PORTB ^= (1 << PORTB5);
			for (e=dot_time; e>0; e--);
			PORTB ^= (1 << PORTB5);
			break;

		}
		for (e=dot_time; e>0; e--);
		morse_str++;
	}
	/* At the end of the letter a silence of 3 dots is expected 3 */
	for (e = dot_time *4; e > 0; e--);

	/* Send the second nibble */
	morse_str = mrs_table[ data & 0x0F ];

	while (*morse_str) {
		signal = *morse_str;
		switch (signal) {
		case '-':
			PORTB ^= (1 << PORTB5);
			for (e=dot_time * 3; e>0; e--);
			PORTB ^= (1 << PORTB5);
			break;

		case '.':
			PORTB ^= (1 << PORTB5);
			for (e=dot_time; e>0; e--);
			PORTB ^= (1 << PORTB5);
			break;

		}
		for (e = dot_time; e > 0; e--);
		morse_str++;
	}
	/* At the end of the letter a silence of 3 dots is expected 3 */
	for (e = dot_time * 4; e > 0; e--);

}
