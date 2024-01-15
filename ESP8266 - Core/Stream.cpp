/*
 Stream.cpp - adds parsing methods to Stream class
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

 Created July 2011
 parsing functions based on TextFinder library by Michael Margolis
 */

#include <Arduino.h>
#include <Stream.h>

#define Parse_TimeOut 1000  // default number of milli-seconds to wait
#define NO_SKIP_CHAR  1  // a magic char not found in a valid ASCII numeric field

// private method to read stream with TimeOut
int Stream::TimedRead() {
    int c;
    _StartMillis = millis();
    do {
        c = Read();
        if(c >= 0)
            return c;
        if(_TimeOut == 0)
            return -1;
        yield();
    } while(millis() - _StartMillis < _TimeOut);
    return -1;     // -1 indicates TimeOut
}

// private method to peek stream with TimeOut
int Stream::TimedPeek() {
    int c;
    _StartMillis = millis();
    do {
        c = Peek();
        if(c >= 0)
            return c;
        if(_TimeOut == 0)
            return -1;
        yield();
    } while(millis() - _StartMillis < _TimeOut);
    return -1;     // -1 indicates TimeOut
}

// returns peek of the next digit in the stream or -1 if TimeOut
// discards non-numeric characters
int Stream::PeekNextDigit(bool detectDecimal) {
    int c;
    while(1) {
        c = TimedPeek();
        if( c < 0 || // TimeOut
            c == '-' ||
            ( c >= '0' && c <= '9' ) ||
            ( detectDecimal && c == '.' ) ) {
            return c;
        }
        Read();  // discard non-numeric
    }
}

// Public Methods
//////////////////////////////////////////////////////////////

void Stream::SetTimeOut(unsigned long TimeOut)  // sets the maximum number of milliseconds to wait
{
    _TimeOut = TimeOut;
}

// Find returns true if the target string is found
bool Stream::Find(const char *target) {
    return FindUntil(target, (char*) "");
}

// reads data from the stream until the target string of given length is found
// returns true if target string is found, false if timed out
bool Stream::Find(const char *target, size_t length) {
    return FindUntil(target, length, NULL, 0);
}

// as Find but search ends if the terminator string is found
bool Stream::FindUntil(const char *target, const char *terminator) {
    return FindUntil(target, strlen(target), terminator, strlen(terminator));
}

// reads data from the stream until the target string of the given length is found
// search terminated if the terminator string is found
// returns true if target string is found, false if terminated or timed out
bool Stream::FindUntil(const char *target, size_t targetLen, const char *terminator, size_t termLen) {
    size_t index = 0;  // maximum target string length is 64k bytes!
    size_t termIndex = 0;
    int c;

    if(*target == 0)
        return true;   // return true if target is a null string
    while((c = TimedRead()) > 0) {

        if(c != target[index])
            index = 0; // reset index if any char does not match

        if(c == target[index]) {
            //////Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
            if(++index >= targetLen) { // return true if all chars in the target match
                return true;
            }
        }

        if(termLen > 0 && c == terminator[termIndex]) {
            if(++termIndex >= termLen)
                return false;       // return false if terminate string found before target string
        } else
            termIndex = 0;
    }
    return false;
}

// returns the first valid (long) integer value from the current position.
// initial characters that are not digits (or the minus sign) are skipped
// function is terminated by the first character that is not a digit.
long Stream::ParseInt() {
    return ParseInt(NO_SKIP_CHAR); // terminate on first non-digit character (or TimeOut)
}

// as above but a given skipChar is ignored
// this allows format characters (typically commas) in values to be ignored
long Stream::ParseInt(char skipChar) {
    boolean isNegative = false;
    long value = 0;
    int c;

    c = PeekNextDigit(false);
    // ignore non numeric leading characters
    if(c < 0)
        return 0; // zero returned if TimeOut

    do {
        if(c == skipChar)
            ; // ignore this character
        else if(c == '-')
            isNegative = true;
        else if(c >= '0' && c <= '9')        // is c a digit?
            value = value * 10 + c - '0';
        Read();  // consume the character we got with peek
        c = TimedPeek();
    } while((c >= '0' && c <= '9') || c == skipChar);

    if(isNegative)
        value = -value;
    return value;
}

// as ParseInt but returns a floating point value
float Stream::ParseFloat() {
    return ParseFloat(NO_SKIP_CHAR);
}

// as above but the given skipChar is ignored
// this allows format characters (typically commas) in values to be ignored
float Stream::ParseFloat(char skipChar) {
    boolean isNegative = false;
    boolean isFraction = false;
    long value = 0;
    int c;
    float fraction = 1.0f;

    c = PeekNextDigit(true);
    // ignore non numeric leading characters
    if(c < 0)
        return 0; // zero returned if TimeOut

    do {
        if(c == skipChar)
            ; // ignore
        else if(c == '-')
            isNegative = true;
        else if(c == '.')
            isFraction = true;
        else if(c >= '0' && c <= '9') {      // is c a digit?
            value = value * 10 + c - '0';
            if(isFraction)
                fraction *= 0.1f;
        }
        Read();  // consume the character we got with peek
        c = TimedPeek();
    } while((c >= '0' && c <= '9') || c == '.' || c == skipChar);

    if(isNegative)
        value = -value;
    if(isFraction)
        return value * fraction;
    else
        return value;
}

// read characters from stream into buffer
// terminates if length characters have been read, or TimeOut (see setTimeOut)
// returns the number of characters placed in the buffer
// the buffer is NOT null terminated.
//
size_t Stream::ReadBytes(char *buffer, size_t length) {
    IAMSLOW();

    size_t count = 0;
    while(count < length) {
        int c = TimedRead();
        if(c < 0)
            break;
        *buffer++ = (char) c;
        count++;
    }
    return count;
}

// as ReadBytes with terminator character
// terminates if length characters have been read, TimeOut, or if the terminator character  detected
// returns the number of characters placed in the buffer (0 means no valid data found)

size_t Stream::ReadBytesUntil(char terminator, char *buffer, size_t length) {
    if(length < 1)
        return 0;
    size_t index = 0;
    while(index < length) {
        int c = TimedRead();
        if(c < 0 || c == terminator)
            break;
        *buffer++ = (char) c;
        index++;
    }
    return index; // return number of characters, not including null terminator
}

String Stream::ReadString() {
    String ret;
    int c = TimedRead();
    while(c >= 0) {
        ret += (char) c;
        c = TimedRead();
    }
    return ret;
}

String Stream::ReadStringUntil(char terminator) {
    String ret;
    int c = TimedRead();
    while(c >= 0 && c != terminator) {
        ret += (char) c;
        c = TimedRead();
    }
    return ret;
}

// read what can be read, immediate exit on unavailable data
// prototype similar to Arduino's `int Client::read(buf, len)`
int Stream::Read (uint8_t* buffer, size_t maxLen)
{
    IAMSLOW();

    size_t nbread = 0;
    while (nbread < maxLen && Available())
    {
        int c = Read();
        if (c == -1)
            break;
        buffer[nbread++] = c;
    }
    return nbread;
}
