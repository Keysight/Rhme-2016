/* * main.c
 *
 *  Created on: Jun 3, 2016
 *      Author: rhme
 */




#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>


#ifdef __AVR__
#include <avr/pgmspace.h>

#else

#include <unistd.h>

#define serial_read(x, y) read(0, x, y)
#define serial_printf(...) printf(__VA_ARGS__);fflush(stdout)
#define serial_init(...)

#endif

#define USART_BAUDRATE 19200
#define BAUD_PRESCALER ( (F_CPU / (USART_BAUDRATE * 16UL)) - 1)


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

	//usart_send_byte('A');

	/* Read byte */
	return UDR0;
}



unsigned char input[0x10] = {0};
uint16_t checks = 0x0;

uint16_t stuff[256]; // Our ROP chain

void readinput(){
	unsigned char c;
	unsigned int idx;
	for(idx=0; idx < sizeof(input)-1; idx++){
		c = usart_recv_byte();
		if (c == '\n' || c == '\r')
			break;
		//usart_print("Recevied\n");
		input[idx] = c;
	}
	input[idx] = 0; // NULL terminate
}

void checklen(){
	unsigned char i = 0;
	for(i=0;input[i]; i++);
	if (i == 13){
		checks |= 1;
	} else {
		checks = 0;
	}

}
void check7(){
	if (input[7] + input[8] == '_'+'t'){
		checks |= (1 << 8);
	} else {
		checks = 0;
	}
}
void dummy1() {
	if (input[7] * input[8] == input[9] + 'k') {
		checks |= (1 << 13);
	} else if (input[1] + input[2] + input[13] == 'e'+'y') {
		checks |= (1 << 2);
		checks |= (1 << 5);
	} else {
		checks |= (0  << 6);
	}
}
void check12(){
	volatile uint8_t i;
	for(i=0;input[i];i++);
	if (input[12] * i == '3'*13){
		checks |= (1 << 13);
	} else {
		checks = 0;
	}
}
void dummy2() {
	if (input[3] + input[5] == input[9] * 'b') {
		checks |= (1 << 12);
	} else if (input[1] + input[2] + input[13] == 'n'*'i') {
		checks |= (0 << 2);
		checks |= (1 << 6);
	} else {
		checks |= (0  << 5);
	}
}
void check9(){
	if (input[9] + input[10] == '0'+'_'){
		checks |= (1 << 10);
	} else {
		checks = 0;
	}
}
void check4(){
	if (input[4] * input[5] == '_'*'1'){
		checks |= (1 << 5);
	} else {
		checks = 0;
	}

}
void dummy3() {
	if (input[11] - input[4] == input[7] * '3') {
		checks |= (1 << 11);
	} else if (input[1] + input[2] + input[13] == '2'*'5') {
		checks |= (1 << 3);
		checks |= (0 << 5);
	} else {
		checks |= (0  << 4);
	}
}
void check3(){
	if (input[3] + input[4] == '3' +'_'){
		checks |= (1 << 4);
	} else {
		checks = 0;
	}

}
void check10(){
	if (input[10] * input[11] == 'm'*'_'){
		checks |= (1 << 11);
	} else {
		checks = 0;
	}
}
void dummy4() {
	if (input[3] << input[2] == input[12] * 'v') {
		checks |= (1 << 10);
	} else if (input[1] + input[2] + input[13] == '6'*'q') {
		checks |= (1 << 13);
		checks |= (1 << 12);
	} else {
		checks |= (0  << 3);
	}
}
void check6(){
	if (input[6] * input[7] == 't'*'_'){
		checks |= (1 << 7);
	} else {
		checks = 0;
	}
}
void check1(){
	if (input[1] + input[2] == 'v' +'1'){
		checks |= (1 << 2);
	} else {
		checks = 0;
	}

}
void check11(){
	if (input[11] + input[12] == 'm'+'3'){
		checks |= (1 << 12);
	} else {
		checks = 0;
	}
}
void check0(){
	if (input[0]*input[1] == 'g'*'1'){
		checks |= (1 << 1);
	} else {
		checks = 0;
	}

}
void check2(){
	if (input[2] * input[3] == 'v'*'3'){
		checks |= (1 << 3);
	} else {
		checks = 0;
	}

}
void check8(){
	if (input[8] * input[9] == 't'*'0'){
		checks |= (1 << 9);
	} else {
		checks = 0;
	}
}
void dummy5() {
	if (input[6] >> input[10] == input[6] * 'm') {
		checks |= (1 << 13);
	} else if (input[1] + input[2] + input[13] == 'y'*'3') {
		checks |= (1 << 12);
		checks |= (0 << 13);
	} else {
		checks |= (0 << 2);
	}
}
void check5(){
	if (input[5] + input[6] == 't'+'1'){
		checks |= (1 << 6);
	} else {
		checks = 0;
	}
}
void dummy6() {
	if (input[2] * input[1] == input[7] * 'u') {
		checks |= (1 << 8);
	} else if (input[4] + input[1] + input[7] == 's'*'r') {
		checks |= (1 << 2);
		checks |= (1 << 5);
	} else {
		checks |= (0  << 1);
	}
}

void final(){
	if ( checks == (1<<14)-1){
		usart_print("\r\nFLAG:D0_you_3ven_ROP?");
		usart_print("\r\n");
	} else {
		usart_print("\r\nBetter luck next time!\r\n");
	}
}

void inline pivot(char *stuff) __attribute__((always_inline));

void inline pivot(char *stuff) {

asm volatile(
//"mov sp, r24\n\t"
"out 0x3D, R24\n\t" // SPL
"out 0x3E, R25\n\t" // SPH
"ret\n\t"
::);
}


void print_text(){
	usart_print("Input: ");
}

volatile uint16_t ctr = 0;
void infloop(){
	while(1);
}

void nop(){
	ctr++;
}


#define SWAP(x) (( (x) >> 8) | (( (x) &  0xFF)<<8))

int main(void) {
	volatile uint16_t idx = 256-40;

	memset(stuff, 0x41, sizeof(stuff));
	serial_init();

	checks = 0;

	//Initialize serial ports
	stuff[idx++] = SWAP((uint16_t)&nop);
	stuff[idx++] = SWAP((uint16_t)&print_text);
	stuff[idx++] = SWAP((uint16_t)&readinput);
	stuff[idx++] = SWAP((uint16_t)&checklen);
	stuff[idx++] = SWAP((uint16_t)&check7);
	stuff[idx++] = SWAP((uint16_t)&check8);
	stuff[idx++] = SWAP((uint16_t)&check0);
	stuff[idx++] = SWAP((uint16_t)&check2);
	stuff[idx++] = SWAP((uint16_t)&check3);
	stuff[idx++] = SWAP((uint16_t)&check6);
	stuff[idx++] = SWAP((uint16_t)&check5);
	stuff[idx++] = SWAP((uint16_t)&check9);
	stuff[idx++] = SWAP((uint16_t)&check1);
	stuff[idx++] = SWAP((uint16_t)&check10);
	stuff[idx++] = SWAP((uint16_t)&check12);
	stuff[idx++] = SWAP((uint16_t)&check4);
	stuff[idx++] = SWAP((uint16_t)&check11);
	stuff[idx++] = SWAP((uint16_t)&final);
	stuff[idx++] = SWAP((uint16_t)&infloop);
	// stuff[idx++] = SWAP((uint16_t)&dummy1);
	// stuff[idx++] = SWAP((uint16_t)&dummy2);
	// stuff[idx++] = SWAP((uint16_t)&dummy3);
	// stuff[idx++] = SWAP((uint16_t)&dummy4);
	// stuff[idx++] = SWAP((uint16_t)&dummy5);
	// stuff[idx++] = SWAP((uint16_t)&dummy6);

	pivot((char *)(&stuff[256-40]) - 1);

	return 0;
}
