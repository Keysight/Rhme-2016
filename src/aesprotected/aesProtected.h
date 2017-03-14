/* aes.h */
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

#ifndef AES_H_
#define AES_H_

#include <stdint.h>

/*
 * The three types defined below are already array types
 * and work as if they are pointers. No need to dereference
 * variables of these types.
 */

typedef uint8_t* aes_state_t;
typedef uint8_t* aes_iv_t;
typedef uint8_t* aes_roundkey_t;
typedef uint8_t* aes_key_t;

#define AES_STATE_SIZE      16
#define AES_IV_SIZE         16
#define AES_ROUNDKEY_SIZE   16
#define AES_KEY_SIZE        (16 * 11)


inline void aes_key_expansion(aes_key_t key);

#if AES_ENCRYPTION

inline void aes_ecb_encrypt(aes_state_t data, aes_key_t key);
inline void aes_cbc_encrypt(aes_state_t data, aes_iv_t, aes_key_t key);

#endif

inline void aes_ecb_decrypt(aes_state_t data, aes_key_t key);
inline void aes_cbc_decrypt(aes_state_t data, aes_iv_t, aes_key_t key);


void delay(uint16_t value);
#define LONGDELAY() delay(0x4FF);

#if ENABLE_RANDOMDELAY
#define RANDOMDELAY() random_delay(0x007F);
//#define SHORTRANDOMDELAY() random_delay(0x001F);
#define SHORTRANDOMDELAY() ;
void random_delay(uint16_t max_value);
#else
#define RANDOMDELAY() ;
#define SHORTRANDOMDELAY() ;
#endif

void executeWithDummies(void (*function)(uint8_t *, uint8_t *), uint8_t *data, uint8_t *key);

#endif
