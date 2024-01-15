/*
 Print.cpp - Base class that provides print() and println()
 Copyright (c) 2008 David A. Mellis.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Modified 23 November 2006 by David A. Mellis
 Modified December 2014 by Ivan Grokhotkov
 Modified May 2015 by Michael C. Miller - esp8266 progmem support
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <Arduino.h>

#include "Print.h"

// Public Methods //////////////////////////////////////////////////////////////

/* default implementation: may be overridden */
size_t PrintClass::Write(const uint8_t *buffer, size_t size) {
    IAMSLOW();

    size_t n = 0;
    while (size--) {
        size_t ret = Write(pgm_read_byte(buffer++));
        if (ret == 0) {
            // Write of last byte didn't complete, abort additional processing
            break;
        }
        n += ret;
    }
    return n;
}

size_t PrintClass::Printf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new (std::nothrow) char[len + 1];
        if (!buffer) {
            return 0;
        }
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    len = Write((const uint8_t*) buffer, len);
    if (buffer != temp) {
        delete[] buffer;
    }
    return len;
}

size_t PrintClass::Printf_P(PGM_P format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf_P(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new (std::nothrow) char[len + 1];
        if (!buffer) {
            return 0;
        }
        va_start(arg, format);
        vsnprintf_P(buffer, len + 1, format, arg);
        va_end(arg);
    }
    len = Write((const uint8_t*) buffer, len);
    if (buffer != temp) {
        delete[] buffer;
    }
    return len;
}

size_t PrintClass::Print(const __FlashStringHelper *ifsh) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);

    char buff[128] __attribute__ ((aligned(4)));
    auto len = strlen_P(p);
    size_t n = 0;
    while (n < len) {
        int to_write = std::min(sizeof(buff), len - n);
        memcpy_P(buff, p, to_write);
        auto written = Write(buff, to_write);
        n += written;
        p += written;
        if (!written) {
            // Some error, Write() should write at least 1 byte before returning
            break;
        }
    }
    return n;
}

size_t PrintClass::Print(const String &s) {
    return Write(s.c_str(), s.length());
}

size_t PrintClass::Print(const char str[]) {
    return Write(str);
}

size_t PrintClass::Print(char c) {
    return Write(c);
}

size_t PrintClass::Print(unsigned char b, int base) {
    return Print((unsigned long) b, base);
}

size_t PrintClass::Print(int n, int base) {
    return Print((long) n, base);
}

size_t PrintClass::Print(unsigned int n, int base) {
    return Print((unsigned long) n, base);
}

size_t PrintClass::Print(long n, int base) {
    int t = 0;
    if (base == 10 && n < 0) {
        t = Print('-');
        n = -n;
    }
    return PrintNumber(static_cast<unsigned long>(n), base) + t;
}

size_t PrintClass::Print(unsigned long n, int base) {
    if (base == 0) {
        return Write(n);
    }
    return PrintNumber(n, base);
}

size_t PrintClass::Print(long long n, int base) {
    int t = 0;
    if (base == 10 && n < 0) {
        t = Print('-');
        n = -n;
    }
    return PrintNumber(static_cast<unsigned long long>(n), base) + t;
}

size_t PrintClass::Print(unsigned long long n, int base) {
    if (base == 0) {
        return Write(n);
    }
    return PrintNumber(n, base);
}

size_t PrintClass::Print(double n, int digits) {
    return PrintNumber(n, digits);
}

size_t PrintClass::Print(const Printable& x) {
    return x.PrintTo(*this);
}

size_t PrintClass::Println(void) {
    return Print("\r\n");
}

size_t PrintClass::Println(const __FlashStringHelper* ifsh) {
    return _Println<const __FlashStringHelper*>(ifsh);
}

size_t PrintClass::Println(const String &s) {
    return _Println(s);
}

size_t PrintClass::Println(const char c[]) {
    return _Println(c);
}

size_t PrintClass::Println(char c) {
    return _Println(c);
}

size_t PrintClass::Println(unsigned char b, int base) {
    return _Println(b, base);
}

size_t PrintClass::Println(int num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(unsigned int num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(long num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(unsigned long num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(long long num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(unsigned long long num, int base) {
    return _Println(num, base);
}

size_t PrintClass::Println(double num, int digits) {
    return _Println(num, digits);
}

size_t PrintClass::Println(const Printable& x) {
    return _Println<const Printable&>(x);
}

// Private Methods /////////////////////////////////////////////////////////////

template<typename T, typename... P> inline size_t PrintClass::_Println(T v, P... args)
{
    size_t n = Print(v, args...);
    n += Println();
    return n;
};

template<typename T> size_t PrintClass::PrintNumber(T n, uint8_t base) {
    char buf[8 * sizeof(n) + 1]; // Assumes 8-bit chars plus zero byte.
    char* str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if (base < 2) {
        base = 10;
    }

    do {
        auto m = n;
        n /= base;
        char c = m - base * n;

        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (n);

    return Write(str);
}

template<> size_t PrintClass::PrintNumber(double number, uint8_t digits) {
    char buf[40];
    return Write(dtostrf(number, 0, digits, buf));
}
