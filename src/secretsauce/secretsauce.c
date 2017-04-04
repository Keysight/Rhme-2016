#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define __AVR_ATmega328P__ 1

#include <avr/io.h>
#include <util/delay.h>

#define USART_BAUDRATE 19200
#define BAUD_PRESCALER ( (F_CPU / (USART_BAUDRATE * 16UL)) - 1)

#include "aes.h"


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

char prbuff[120];
void serial_printf(const char *format, ...) 
{
	va_list args;

	va_start(args, format);
	vsnprintf(prbuff, 120, format, args);
	usart_print(prbuff);
	va_end(args);
}

static uint8_t append_char(char *str, uint8_t ch, uint8_t k, uint8_t max) 
{
	usart_send_byte(ch);

	if (k < max - 1)
		str[k++] = ch;
	return k;
}

uint8_t usart_read_str(char *str, uint8_t max) {
	uint8_t k, eos, ch; // , ch2
	k = 0;          // index of first free position in the string
	eos = 0;        // end of string, used as boolean
	while (!eos) {
		ch = usart_recv_byte();
		if (ch == '\r') {
			eos = 1;
		} else if (ch == '\n') {
			// do nothing
		} else {
			k = append_char(str, ch, k, max);
		}
	}

	// terminate string accordingly
	str[k] = '\0';

	/* return input length */
	return k;
}


// Initializes ACD to read the PINS
void InitADC()
{
	ADMUX=(1<<REFS0);                         // For Aref=AVcc;
	ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Rrescalar div factor =128
}

// Takes a number of a PIN between 0x00 and 0x07
// Rerurns the value of the input on the PIN
uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel ch must be 0-7
	ch=ch&0b00000111;
	ADMUX|=ch;
	//Start Single conversion
	ADCSRA|=(1<<ADSC);
	//Wait for conversion to complete
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	return(ADC);
}

// Checks a password and leaks a timing information
int compPasswords(char* pass, char* inp)
{
	int i;
	if (inp[0] == '\r' || inp[0] == '\n') {
		inp++;
	}
	if (strlen(pass) != strlen(inp)) {
		//serial_printf("Wrong");
		return 0;
	}
	for (i = 0; i < strlen(pass); i++) {
		_delay_ms(1);
		if (pass[i] != inp[i]) {
			return 0;
		}
	}
	return 1;

}

void delayVar(uint8_t d)
{
	int i;
	for (i = 0; i < d; i++)	{
		_delay_ms(1);
	}
}

// Generates a super true random nonce by reading Analog Pin 3
// After six readings the array is encrypted using AES
// Returns 16 bytes random nonce
void getNonce(uint8_t *output)
{
	aes_key_t key;
	key = (uint8_t*)malloc(AES_KEY_SIZE);

	key[0] = ReadADC(0x03);

	_delay_ms(1);
	key[1] = ReadADC(0x03);

	_delay_ms(1);
	key[2] = ReadADC(0x03);

	_delay_ms(1);
	key[3] = ReadADC(0x03);

	_delay_ms(1);
	key[4] = ReadADC(0x03);

	_delay_ms(1);
	key[5] = ReadADC(0x03);

	_delay_ms(1);
	key[6] = ReadADC(0x03);

	key[ 7] = 0;
	key[ 8] = 0;
	key[ 9] = 0;
	key[10] = 0;
	key[11] = 0;
	key[12] = 0;
	key[13] = 0;
	key[14] = 0;
	key[15] = 0;

	output[ 0] = key[0];
	output[ 1] = key[1];
	output[ 2] = key[2];
	output[ 3] = key[3];
	output[ 4] = key[4];
	output[ 5] = key[5];
	output[ 6] = key[6];
	output[ 7] = 0;
	output[ 8] = 0;
	output[ 9] = 0;
	output[10] = 0;
	output[11] = 0;
	output[12] = 0;
	output[13] = 0;
	output[14] = 0;
	output[15] = 0;

	aes_key_expansion(key);
	aes_ecb_encrypt(output, key);

	free(key);
}


int main(void)
{
	serial_init();
	_delay_ms(10);

	char secretPass[16] = {'T', 'I', 'm', 'I', 'n', 'G', '@', 't', 't', 'A', 'k', 'w', '0', 'r', 'k', 0x00}; // -- RANDOMIZE later
	char inputBuff[32] = {0};
	unsigned char authL0 = 0;
	char flag[16] = {'T', 'o', '0', '_', 'm', 'u', 'c', 'h', '_', '3', 'n', 't', 'r', 'o', 'p', 'y'};

	int inpLen = 0;
	int i;
	int j;

	InitADC();

	uint8_t in[50] = { 0x00 };
	uint8_t iv[16] = {0};
	uint8_t enc[16] = {0};
	uint8_t res[50] = {0};

	aes_key_t key;
	key = (uint8_t*)malloc(AES_KEY_SIZE);
	key[ 0] = 0x39;
	key[ 1] = 0x27;
	key[ 2] = 0xCD;
	key[ 3] = 0x37;
	key[ 4] = 0xC4;
	key[ 5] = 0xAF;
	key[ 6] = 0xE4;
	key[ 7] = 0x6C;
	key[ 8] = 0xFA;
	key[ 9] = 0xC2;
	key[10] = 0xAB;
	key[11] = 0x3E;
	key[12] = 0x21;
	key[13] = 0xAA;
	key[14] = 0x4E;
	key[15] = 0x71;
	aes_key_expansion(key);


	if (authL0 == 0) {
		serial_printf("Welcome to Secure Encryption System(SES)!\n");
		serial_printf("Authentication step.\n");
		serial_printf("Input provided secret password.\n");
		serial_printf("If you lost your password call the customer service.\n");

		while (1) {
			serial_printf(">");
			inpLen = usart_read_str(inputBuff, 20);
			if (inpLen < 20) {
				inputBuff[inpLen] = '\0';

			} else {
				inputBuff[19] = '\0';
			}

			serial_printf("\nChecking password...\n");
			if (compPasswords(secretPass, inputBuff) == 0)	{
				serial_printf("Password is incorrect!\n");

			} else {
				serial_printf("Password is correct!\n");
				authL0 = 7;
				_delay_ms(500);
				break;
			}
		}
	}

	if (authL0 == 7) {
		serial_printf("\n\n************************************************\n");
		serial_printf("Authentication complete. Welcome to the system!\n");
		serial_printf("Now you can encrypt messages up to 32 bytes.\n");

		while (1) {
			serial_printf("Input data to encrypt:\n");
			serial_printf("> ");
			inpLen = usart_read_str((char *) in, 33); // read up to 32 byte input

			// Append secret flag after
			for (i = 0; i < 16; i++) {
				in[i+inpLen] = flag[i];
			}

			// Zero the rest just in case
			for (i = (16 + inpLen); i < 50; i++) {
				in[i] = 0x00;
			}

			getNonce(iv);

			serial_printf("\nTrue Random Nonce:\t");
			for (i = 0; i < 16; i++) {
				serial_printf("%02x", iv[i]);
			}
			serial_printf("\n");

			uint8_t block = 0;
			for (i = 0; i < (16 + inpLen); i+=16) {
				iv[15] ^= block; // IV XORed with a block counter which is at most 3.
				memcpy(enc, iv, 16);
				aes_ecb_encrypt(enc, key);
				for (j = 0; j < 16; j++) {
					res[block*16+j] = enc[j] ^ in[block*16+j];
				}
				block++;
			}

			serial_printf("Encryption:\t");
			for (i = 0; i < block*16; i++) {
				serial_printf("%02x", res[i]);
			}
			serial_printf("\n");
		}
	}
	free(key);
	return 0;
}
