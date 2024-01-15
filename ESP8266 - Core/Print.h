/*
 Print.h - Base class that provides Print() and Println()
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
 License along with this library; if not, Write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef Print_h
#define Print_h

#include <stdint.h>
#include <stddef.h>

#include "WString.h"
#include "Printable.h"

#include "stdlib_noniso.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class PrintClass {
    private:
        int Write_error = 0;
        template<typename T> size_t PrintNumber(T n, uint8_t base);
        template<typename T, typename... P> inline size_t _Println(T v, P... args);
    protected:
        void SetWriteError(int err = 1) {
            Write_error = err;
        }
    public:
        PrintClass() {}

        int GetWriteError() {
            return Write_error;
        }
        void ClearWriteError() {
            SetWriteError(0);
        }

        virtual size_t Write(uint8_t) = 0;

        size_t Write(const char *str) 
        {
            if(str == NULL)
                return 0;
            return Write((const uint8_t *) str, strlen_P(str));
        }
        virtual size_t Write(const uint8_t *buffer, size_t size);
        size_t Write(const char *buffer, size_t size) {
            return Write((const uint8_t *) buffer, size);
        }
        // These handle ambiguity for Write(0) case, because (0) can be a pointer or an integer
        inline size_t Write(short t) { return Write((uint8_t)t); }
        inline size_t Write(unsigned short t) { return Write((uint8_t)t); }
        inline size_t Write(int t) { return Write((uint8_t)t); }
        inline size_t Write(unsigned int t) { return Write((uint8_t)t); }
        inline size_t Write(long t) { return Write((uint8_t)t); }
        inline size_t Write(unsigned long t) { return Write((uint8_t)t); }
        inline size_t Write(long long t) { return Write((uint8_t)t); }
        inline size_t Write(unsigned long long t) { return Write((uint8_t)t); }
        // Enable Write(char) to fall through to Write(uint8_t)
        inline size_t Write(char c) { return Write((uint8_t) c); }
        inline size_t Write(int8_t c) { return Write((uint8_t) c); }

        // default to zero, meaning "a single Write may block"
        // should be overridden by subclasses with buffering
        virtual int AvailableForWrite() { return 0; }

        size_t Printf(const char * format, ...)  __attribute__ ((format (printf, 2, 3)));
        size_t Printf_P(PGM_P format, ...) __attribute__((format(printf, 2, 3)));
        size_t Print(const __FlashStringHelper *);
        size_t Print(const String &);
        size_t Print(const char[]);
        size_t Print(char);
        size_t Print(unsigned char, int = DEC);
        size_t Print(int, int = DEC);
        size_t Print(unsigned int, int = DEC);
        size_t Print(long, int = DEC);
        size_t Print(unsigned long, int = DEC);
        size_t Print(long long, int = DEC);
        size_t Print(unsigned long long, int = DEC);
        size_t Print(double, int = 2);
        size_t Print(const Printable&);

        size_t Println(const __FlashStringHelper *);
        size_t Println(const String &s);
        size_t Println(const char[]);
        size_t Println(char);
        size_t Println(unsigned char, int = DEC);
        size_t Println(int, int = DEC);
        size_t Println(unsigned int, int = DEC);
        size_t Println(long, int = DEC);
        size_t Println(unsigned long, int = DEC);
        size_t Println(long long, int = DEC);
        size_t Println(unsigned long long, int = DEC);
        size_t Println(double, int = 2);
        size_t Println(const Printable&);
        size_t Println(void);

        // flush():
        // Empty implementation by default in Print::
        // should wait for all outgoing characters to be sent, output buffer is empty after this call
        virtual void Flush() { }

        // by default Write timeout is possible (outgoing data from network,serial..)
        // (children can override to false (like String))
        virtual bool OutPutCanTimeOut () { return true; }
};

template<> size_t PrintClass::PrintNumber(double number, uint8_t digits);

#endif
