/*
 wiring_Shift.c - ShiftOut() function
 Part of Arduino - http://www.arduino.cc/

 Copyright (c) 2005-2006 David A. Mellis

 Note: file renamed with a core_esp8266_ prefix to simplify linker
 script rules for moving code into irom0_text section.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General
 Public License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA  02111-1307  USA

 $Id: wiring.c 248 2007-02-03 15:36:30Z mellis $
 */

#include "wiring_private.h"
extern "C" {

uint8_t ShiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i) {
        DigitalWrite(clockPin, HIGH);
        if(bitOrder == LSBFIRST)
            value |= DigitalRead(dataPin) << i;
        else
            value |= DigitalRead(dataPin) << (7 - i);
        DigitalWrite(clockPin, LOW);
    }
    return value;
}

void ShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
    uint8_t i;

    for(i = 0; i < 8; i++) {
        if(bitOrder == LSBFIRST)
            DigitalWrite(dataPin, !!(val & (1 << i)));
        else
            DigitalWrite(dataPin, !!(val & (1 << (7 - i))));

        DigitalWrite(clockPin, HIGH);
        DigitalWrite(clockPin, LOW);
    }
}

};
