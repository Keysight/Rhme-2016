/* RHme2
 * Side Channel Analysis - Piece of SCAke
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "serial_io.h"
#include "aesProtected.h"

//For most platforms we want to use the AVR ADC-pins, since they have a seperate power rail
//This can be overridden elsewhere
#if TRIGGER==1
#define trigger_setup() DDRB = 0x20;
#define trigger_high()  PORTB |= (1 << PORTB5);
#define trigger_low()   PORTB &= ~(1 << PORTB5);
#else
#define trigger_setup()
#define trigger_low()
#define trigger_high()
#endif

uint8_t key[AES_KEY_SIZE];
uint8_t buf[AES_STATE_SIZE];

void fill_buf() 
{
    uint8_t i;

    for(i = 0; i < 16; ++i) {
        buf[i] = usart_recv_byte();
    }
}

void send_buf() 
{
    uint8_t i;

    for(i = 0; i < 16; ++i) {
        usart_send_byte(buf[i]);
    }
}

void encrypt() 
{
    trigger_high();
    aes_ecb_encrypt(buf, key);
    trigger_low();
}

void decrypt() 
{
    trigger_high();
    aes_ecb_decrypt(buf, key);
    trigger_low();
}

void aes_setup() 
{
    uint8_t aux[32] = {0xaf, 0x23, 0xd5, 0x45, 0xa0, 0xea, 0xe6, 0xa0, 0x74, 0x65, 0x96, 0xca, 0xce, 0x51, 0xf0, 0xf7};
    int i;

    for (i = 0; i < AES_KEY_SIZE; i++) {
        key[i] = aux[i];
    }

    memset(aux, 0, sizeof(aux));
    aes_key_expansion(key);
}

void random_setup() 
{
    static uint8_t initialized = 0;
    // We XOR the stored seed with  the input buffer for adding more entropy
    // (assuming somebody doing SCA and randomizing the input)
    if (initialized == 0) {
        srand(eeprom_read_word(0) ^ (buf[0]<<8 | buf[1]));
        initialized = 1;
    }

}

int main(void) 
{
    uint8_t command;

    serial_init();
    trigger_setup();
    aes_setup();

    while(1) {
        command = usart_recv_byte();

        switch(command) {
            case 'e':
                fill_buf();
                random_setup();
                encrypt();
                send_buf();
                break;

            case 'd':
                fill_buf();
                random_setup();
                decrypt();
                send_buf();
                
                break;
            default:
                continue;
        }
    }

    return 0;
}
