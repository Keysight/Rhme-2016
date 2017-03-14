/* aes.c */
/*
 * This code is/was initially part of the AVR-Crypto-Lib. It is now
 * stripped down such that only AES 128 encryption and decryption in
 * CBC mode is supported.
 *
 * Copyright (C) 2016      alegen (alex@alegen.net)
 * Copyright (C) 2006-2016 Daniel Otte (bg@nerilex.org)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "aesProtected.h"
#include <avr/pgmspace.h>
#include "gf256mul.h"

/*
 * Function prototypes which are not public.
 */

static void aes_rotword(uint32_t* a); 

#if AES_ENCRYPTION

static void aes_enc_round(aes_state_t data, aes_roundkey_t k);
static void aes_enc_lastround(aes_state_t data, aes_roundkey_t k);
static void aes_shiftrow(uint8_t* row_start, uint8_t shift);

#endif

static void aes_dec_firstround(aes_state_t data, aes_roundkey_t k);
static void aes_dec_round(aes_state_t data, aes_roundkey_t k);
static void aes_invshiftrow(uint8_t* row_start, uint8_t shift);

/*
 * Tables with precomputed values.
 */

const uint8_t PROGMEM aes_sbox[256] = { 
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 
    0xb0, 0x54, 0xbb, 0x16}; 

const uint8_t PROGMEM aes_invsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
    0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
    0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
    0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
    0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
    0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
    0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
    0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
    0x55, 0x21, 0x0c, 0x7d};

const uint8_t PROGMEM rc_tab[] = { 
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c}; 

/*
 * AES key expansion code
 */

void aes_key_expansion(aes_key_t key) { 
    uint8_t i; 
    // number of columns (32 bit words) comprising the state 
    // in generic AES, this is constant to 4 
    uint8_t nb = 4; 
    // number of 32 bit words comprising the cipher key 
    // 4, 6 or 8, but 4 in our case 
    uint8_t nk = 4; 
    // number of rounds which is either 10, 12 or 14, but 10 in our case 
    uint8_t nr = 10; 
    // union which lets us select easily between 32 bit 
    // words and individual bytes 
    union { 
        uint32_t v32; 
        uint8_t v8[4]; 
    } tmp; 
    for (i = nk; i < nb * (nr + 1); i++) { 
        SHORTRANDOMDELAY();
        tmp.v32 = ((uint32_t*)key)[i - 1]; 
        if ( i % nk == 0) { 
            aes_rotword(&(tmp.v32)); 
            tmp.v8[0] = pgm_read_byte(aes_sbox + tmp.v8[0]); 
            tmp.v8[1] = pgm_read_byte(aes_sbox + tmp.v8[1]); 
            tmp.v8[2] = pgm_read_byte(aes_sbox + tmp.v8[2]); 
            tmp.v8[3] = pgm_read_byte(aes_sbox + tmp.v8[3]); 
            tmp.v8[0] ^= pgm_read_byte(rc_tab + i / nk); 
        } 
        ((uint32_t*)key)[i] = ((uint32_t*)key)[i - nk] ^ tmp.v32; 
    } 
} 

static void aes_rotword(uint32_t* a) {
    uint8_t t;
    t = ((uint8_t*)a)[0];
    ((uint8_t*)a)[0] = ((uint8_t*)a)[1];
    ((uint8_t*)a)[1] = ((uint8_t*)a)[2];
    ((uint8_t*)a)[2] = ((uint8_t*)a)[3];
    ((uint8_t*)a)[3] = t;
}

/*
 * AES encryption code.
 */

#if AES_ENCRYPTION

void aes_ecb_encrypt_inner(aes_state_t data, aes_key_t key) {
    uint8_t i;
    uint8_t rounds = 10;
  
    RANDOMDELAY();
    for (i = 0; i < 16; ++i) {
        data[i] ^= key[0 * AES_ROUNDKEY_SIZE + i];
        SHORTRANDOMDELAY();
    }

    i = 1;
    for (; rounds > 1; --rounds) {
        RANDOMDELAY();
       
       executeWithDummies(&aes_enc_round, data, key + i * AES_ROUNDKEY_SIZE);

        ++i;
    }

    RANDOMDELAY();

    executeWithDummies(&aes_enc_lastround, data, key + i * AES_ROUNDKEY_SIZE);    

    RANDOMDELAY();

}


void aes_ecb_encrypt(aes_state_t data, aes_key_t key) {
    #if ENABLE_ANTIDFA
    uint8_t bufBck[AES_STATE_SIZE];
    aes_state_t dataBck = bufBck;
    memcpy(dataBck, data, AES_STATE_SIZE);    
    #endif

    aes_ecb_encrypt_inner(data, key);        

    #if ENABLE_ANTIDFA

    LONGDELAY();
    aes_ecb_encrypt_inner(dataBck, key);        

    if (memcmp(data,dataBck,AES_STATE_SIZE) != 0) {
        memset(data,0,AES_STATE_SIZE);  //Fault detected. Clearing the output
    }

    #endif
}

 void aes_cbc_encrypt(aes_state_t data, aes_iv_t iv, aes_key_t key) {
    uint8_t i;
    for (i = 0; i < AES_STATE_SIZE; i++)
        data[i] = data[i] ^ iv[i];
    aes_ecb_encrypt(data, key);
}

static void aes_enc_round(aes_state_t data, aes_roundkey_t k) {
    uint8_t tmp[16], t;
    uint8_t i;

    // sub bytes
    for (i = 0; i < 16; ++i) {
        tmp[i] = pgm_read_byte(aes_sbox + data[i]);
    }

    // shift rows
    SHORTRANDOMDELAY();
    aes_shiftrow(tmp + 1, 1);
    SHORTRANDOMDELAY();    
    aes_shiftrow(tmp + 2, 2);
    SHORTRANDOMDELAY();
    aes_shiftrow(tmp + 3, 3);
    SHORTRANDOMDELAY();

    // mix colums
    for (i = 0; i < 4; ++i) {
        t = tmp[4 * i + 0] ^ tmp[4 * i + 1] ^ tmp[4 * i + 2] ^ tmp[4 * i + 3];        
        SHORTRANDOMDELAY();
        data[4 * i + 0] = gf256mul(2, tmp[4 * i + 0] ^ tmp[4 * i + 1], 0x1b) ^
                          tmp[4 * i + 0] ^ t;
        SHORTRANDOMDELAY();                          
        data[4 * i + 1] = gf256mul(2, tmp[4 * i + 1] ^ tmp[4 * i + 2], 0x1b) ^
                          tmp[4 * i + 1] ^ t;
        SHORTRANDOMDELAY();                          
        data[4 * i + 2] = gf256mul(2, tmp[4 * i + 2] ^ tmp[4 * i + 3], 0x1b) ^
                          tmp[4 * i + 2] ^ t;
        SHORTRANDOMDELAY();
        data[4 * i + 3] = gf256mul(2, tmp[4 * i + 3] ^ tmp[4 * i + 0], 0x1b) ^
                          tmp[4 * i + 3] ^ t;
        SHORTRANDOMDELAY();                          
    }

    // add key
    for (i = 0; i < 16; ++i) {
        data[i] ^= k[i];
        SHORTRANDOMDELAY();
    }
}

static void aes_enc_lastround(aes_state_t data, aes_roundkey_t k) {
    uint8_t i;

    // sub bytes
    for (i = 0; i < 16; ++i) {
        data[i] = pgm_read_byte(aes_sbox + data[i]);
    }

    // shift rows
    SHORTRANDOMDELAY();
    aes_shiftrow(data + 1, 1);
    SHORTRANDOMDELAY();
    aes_shiftrow(data + 2, 2);
    SHORTRANDOMDELAY();    
    aes_shiftrow(data + 3, 3);
    SHORTRANDOMDELAY();    

    // add key
    for (i = 0; i < 16; ++i) {
        data[i] ^= k[i];
        SHORTRANDOMDELAY();
    }
}

static void aes_shiftrow(uint8_t* row_start, uint8_t shift) {
    uint8_t row[4];
    row[0] = row_start[ 0];
    row[1] = row_start[ 4];
    row[2] = row_start[ 8];
    row[3] = row_start[12];
    row_start[ 0] = row[ (shift + 0) & 3 ];
    row_start[ 4] = row[ (shift + 1) & 3 ];
    row_start[ 8] = row[ (shift + 2) & 3 ];
    row_start[12] = row[ (shift + 3) & 3 ];
}

#endif // AES_ENCRYPTION

/*
 * AES decryption code.
 */

 void aes_ecb_decrypt_inner(aes_state_t data, aes_key_t key) {
    uint8_t i;
    uint8_t rounds = 10;
    i = rounds;
    RANDOMDELAY();
    executeWithDummies(&aes_dec_firstround,data, key + i * AES_ROUNDKEY_SIZE);    
    for (; rounds > 1; --rounds) {
        --i;
        RANDOMDELAY();
        executeWithDummies(&aes_dec_round,data, key + i * AES_ROUNDKEY_SIZE);            
    }
    RANDOMDELAY();
    for (i = 0; i < 16; ++i) {
        SHORTRANDOMDELAY();        
        data[i] ^= key[0 * AES_ROUNDKEY_SIZE + i];
    }
    RANDOMDELAY();
}


void aes_ecb_decrypt(aes_state_t data, aes_key_t key) {
    #if ENABLE_ANTIDFA
    uint8_t bufBck[AES_STATE_SIZE];
    aes_state_t dataBck = bufBck;        
    memcpy(dataBck, data, AES_STATE_SIZE);    
    
    #endif

    aes_ecb_decrypt_inner(data, key);    

    #if ENABLE_ANTIDFA

    LONGDELAY();    
    
    aes_ecb_decrypt_inner(dataBck, key); 
   
    if (memcmp(data,dataBck,AES_STATE_SIZE) != 0) {
        memset(data,0,AES_STATE_SIZE);  //Fault detected. Clearing the output
    }

    #endif
}


 void aes_cbc_decrypt(aes_state_t data, aes_iv_t iv, aes_key_t key) {
    uint8_t i;
    aes_ecb_decrypt(data, key);
    for (i = 0; i < AES_STATE_SIZE; i++)
        data[i] = data[i] ^ iv[i];
}

static void aes_dec_firstround(aes_state_t data, aes_roundkey_t k) {
    uint8_t i;

    // add key
    for (i = 0; i < 16; ++i) {
        SHORTRANDOMDELAY();        
        data[i] ^= k[i];
    }

    // shift rows
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 1, 1);
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 2, 2);
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 3, 3);
    SHORTRANDOMDELAY();

    // sub bytes
    for (i = 0; i < 16; ++i) {
        data[i] = pgm_read_byte(aes_invsbox + data[i]);
    }
}

static void aes_dec_round(aes_state_t data, aes_roundkey_t k) {
    uint8_t tmp[16];
    uint8_t i;
    uint8_t t, u, v, w;

    // add key
    for (i = 0; i < 16; ++i) {
        SHORTRANDOMDELAY();
        tmp[i] = data[i] ^ k[i];
    }

    // mix colums
    for (i = 0; i < 4; ++i) {
        SHORTRANDOMDELAY();
        t = tmp[4 * i + 3] ^ tmp[4 * i + 2];
        SHORTRANDOMDELAY();
        u = tmp[4 * i + 1] ^ tmp[4 * i + 0];
        SHORTRANDOMDELAY();
        v = t ^ u;
        SHORTRANDOMDELAY();
        v = gf256mul(0x09, v, 0x1b);
        SHORTRANDOMDELAY();
        w = v ^ gf256mul(0x04, tmp[4 * i + 2] ^ tmp[4 * i + 0], 0x1b);
        SHORTRANDOMDELAY();   
        v = v ^ gf256mul(0x04, tmp[4 * i + 3] ^ tmp[4 * i + 1], 0x1b);
        SHORTRANDOMDELAY();
        data[4 * i + 3] = tmp[4 * i + 3] ^ v ^
                          gf256mul(0x02, tmp[4 * i + 0] ^ tmp[4 * i + 3], 0x1b);
        SHORTRANDOMDELAY();
        data[4 * i + 2] = tmp[4 * i + 2] ^ w ^ gf256mul(0x02, t, 0x1b);
        SHORTRANDOMDELAY();
        data[4 * i + 1] = tmp[4 * i + 1] ^ v ^
                          gf256mul(0x02, tmp[4 * i + 2] ^ tmp[4 * i + 1], 0x1b);
        SHORTRANDOMDELAY();                     
        data[4 * i + 0] = tmp[4 * i + 0] ^ w ^ gf256mul(0x02, u, 0x1b);        
    }

    // shift rows
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 1, 1);
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 2, 2);
    SHORTRANDOMDELAY();
    aes_invshiftrow(data + 3, 3);
    SHORTRANDOMDELAY();

    // sub bytes
    for (i = 0; i < 16; ++i) {
        data[i] = pgm_read_byte(aes_invsbox + data[i]);
    }
}

static void aes_invshiftrow(uint8_t* row_start, uint8_t shift) {
    uint8_t row[4];
    row[0] = row_start[ 0];
    row[1] = row_start[ 4];
    row[2] = row_start[ 8];
    row[3] = row_start[12];
    row_start[ 0] = row[ (4 - shift + 0) & 3 ];
    row_start[ 4] = row[ (4 - shift + 1) & 3 ];
    row_start[ 8] = row[ (4 - shift + 2) & 3 ];
    row_start[12] = row[ (4 - shift + 3) & 3 ];
}


#if ENABLE_RANDOMDELAY


void random_delay(uint16_t max_value) {

    volatile uint16_t n = rand() % max_value;    
    
    for (; n > 0; n--) {                                               \
        asm volatile("nop");                                                \
    }
}


#endif

void delay(uint16_t value) { 
    volatile uint16_t n =value;    
    
    for (; n > 0; n--) {                                               \
        asm volatile("nop");                                                \
    }
}

void executeWithDummies(void (*function)(uint8_t *, uint8_t *), uint8_t *data, uint8_t *key) {

    #if ENABLE_DUMMYROUNDS     

    uint8_t randomData[16+16];    
    uint8_t i, j;

    uint8_t numDummies = 1 + rand()%3;
    uint8_t posRealOp = rand()%numDummies;
    
    for (i=0; i<numDummies; i++) {
        
        // Fill the buffer randomly
        for (j=0; j<32; j+=2)
            *(randomData+j)=rand();

        if (i==posRealOp)
            function(data, key);
        else
            function(randomData, randomData+16);
    }

    #else
    function(data, key);
    #endif

}

