/* gf256mul.S */
/*
 * This file is part of the AVR-Crypto-Lib.
 * Copyright (C) 2006-2015 Daniel Otte (bg@nerilex.org)
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

 /*
  * For reference, the meaning of the 'f' character in behind labels in branch
  * instruction refers to local labels. Read more about it here:
  * http://www.atmel.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_asmstabs.html
  *
  * The algorithm used in this implementation is the Russian Peasant
  * Multiplication algorithm. Read more about it here:
  * https://en.wikipedia.org/wiki/Ancient_Egyptian_multiplication
  */

#include <avr/io.h>
#define OPTIMIZE_SMALL_A

/*
 * param a:         r24
 * param b:         r22
 * param reducer:   r20
 * return value:    r24
 */

/*
 * aliases for registers
 */
A = 23
B = 22
P = 24

.global gf256mul

#ifdef OPTIMIZE_SMALL_A

gf256mul:
    ; A = r24
    mov A, r24
    ; r24 = 0
    clr r24
1:
    ; A >> 1
    ; bit 0 is loaded into the C flag
    ; if result is 0, the Z flag is set
    lsr A
    ; branch if Z flag is set
    breq 4f
    ; branch if C flag is cleared
    brcc 2f
    ; P = P xor B
    eor P, B
2:
    ; B << 1
    ; bit 7 is loaded into the C flag
    ; if result is 0, the Z flag is set
    lsl B
    ; branch if C flag is cleared
    brcc 3f
    ; B = B xor r20
    eor B, r20
3:
    rjmp 1b
4:
    ; branch if C flag is cleared
    brcc 2f
    ; P = P xor B
    eor P, B
2:
    ret

#else // OPTIMIZE_SMALL_A not defined

gf256mul:
    ; A = r24
    mov A, r24
    ; r24 = 0
    clr r24
    ; r25 = 0
    ldi r25, 8
1:
    ; A >> 1
    ; bit 0 is loaded into the C flag
    ; if result is 0, the Z flag is set
    lsr A
    ; branch if C flag is cleared
    brcc 2f
    ; P = P xor B
    eor P, B
2:
    ; B << 1
    lsl B
    ; branch if C flag is cleared
    brcc 3f
    ; B = B xor r20
    eor B, r20
3:
    ; r25 -= 1
    ; if result is 0, the Z flag is set
    dec r25
    ; branch if Z flag is not set
    brne 1b
    ret

#endif // OPTIMIZE_SMALL_A
