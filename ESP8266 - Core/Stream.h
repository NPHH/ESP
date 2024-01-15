/*
 Stream.h - base class for character-based Streams.
 Copyright (c) 2010 David A. Mellis.  All right reserved.

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

 parsing functions based on TextFinder library by Michael Margolis
 */

#ifndef Stream_h
#define Stream_h

#include <debug.h>
#include <inttypes.h>
#include <Print.h>
#include <PolledTimeOut.h>
#include <sys/types.h> // ssize_t

// compatibility macros for testing
/*
 #define   getInt()            ParseInt()
 #define   getInt(skipChar)    ParseInt(skipchar)
 #define   getFloat()          ParseFloat()
 #define   getFloat(skipChar)  ParseFloat(skipChar)
 #define   getString( pre_string, post_string, buffer, length)
 ReadBytesBetween( pre_string, terminator, buffer, length)
 */

// Arduino `Client: public Stream` class defines `virtual int read(uint8_t *buf, size_t size) = 0;`
// This function is now imported into `Stream::` for `Stream::Send*()`.
// Other classes inheriting from `Stream::` and implementing `read(uint8_t *buf, size_t size)`
// must consequently use `int` as return type, namely Hardware/SoftwareSerial, FileSystems...
#define Stream_READ_RETURNS_INT 1

// Stream::Send API is present
#define StreamSend_API 1

class Stream: public PrintClass {
    protected:
        unsigned long _TimeOut = 1000;  // number of milliseconds to wait for the next char before aborting timed read
        unsigned long _StartMillis;  // used for TimeOut measurement
        int TimedRead();    // private method to read Stream with TimeOut
        int TimedPeek();    // private method to Peek Stream with TimeOut
        int PeekNextDigit(bool detectDecimal = false); // returns the next numeric digit in the Stream or -1 if TimeOut

    public:
        virtual int Available() = 0;
        virtual int Read() = 0;
        virtual int Peek() = 0;

        Stream() {}
        virtual ~Stream() {}

// parsing methods

        void SetTimeOut(unsigned long TimeOut);  // sets maximum milliseconds to wait for Stream data, default is 1 second
        unsigned long GetTimeOut () const { return _TimeOut; }

        bool Find(const char *target);   // reads data from the Stream until the target string is found
        bool Find(uint8_t *target) {
            return Find((char *) target);
        }
        // returns true if target string is found, false if timed out (see setTimeOut)

        bool Find(const char *target, size_t length);   // reads data from the Stream until the target string of given length is found
        bool Find(const uint8_t *target, size_t length) {
            return Find((char *) target, length);
        }
        // returns true if target string is found, false if timed out

        bool Find(char target) { return Find (&target, 1); }

        bool FindUntil(const char *target, const char *terminator);   // as Find but search ends if the terminator string is found
        bool FindUntil(const uint8_t *target, const char *terminator) {
            return FindUntil((char *) target, terminator);
        }

        bool FindUntil(const char *target, size_t targetLen, const char *terminate, size_t termLen);   // as above but search ends if the terminate string is found
        bool FindUntil(const uint8_t *target, size_t targetLen, const char *terminate, size_t termLen) {
            return FindUntil((char *) target, targetLen, terminate, termLen);
        }

        long ParseInt(); // returns the first valid (long) integer value from the current position.
        // initial characters that are not digits (or the minus sign) are skipped
        // integer is terminated by the first character that is not a digit.

        float ParseFloat();               // float version of ParseInt

        virtual size_t ReadBytes(char *buffer, size_t length); // read chars from Stream into buffer
        virtual size_t ReadBytes(uint8_t *buffer, size_t length) {
            return ReadBytes((char *) buffer, length);
        }
        // terminates if length characters have been read or TimeOut (see setTimeOut)
        // returns the number of characters placed in the buffer (0 means no valid data found)

        size_t ReadBytesUntil(char terminator, char *buffer, size_t length); // as ReadBytes with terminator character
        size_t ReadBytesUntil(char terminator, uint8_t *buffer, size_t length) {
            return ReadBytesUntil(terminator, (char *) buffer, length);
        }
        // terminates if length characters have been read, TimeOut, or if the terminator character  detected
        // returns the number of characters placed in the buffer (0 means no valid data found)

        // Arduino String functions to be added here
        virtual String ReadString();
        String ReadStringUntil(char terminator);

        virtual int Read (uint8_t* buffer, size_t len);
        int Read (char* buffer, size_t len) { return Read((uint8_t*)buffer, len); }

        //////////////////// extension: direct access to input buffer
        // to provide when possible a pointer to available data for read

        // informs user and ::to*() on effective buffered Peek API implementation
        // by default: not available
        virtual bool HasPeekBufferAPI () const { return false; }

        // returns number of byte accessible by PeekBuffer()
        virtual size_t PeekAvailable () { return 0; }

        // returns a pointer to available data buffer (size = PeekAvailable())
        // semantic forbids any kind of ::read()
        //     - after calling PeekBuffer()
        //     - and before calling PeekConsume()
        virtual const char* PeekBuffer () { return nullptr; }

        // consumes bytes after PeekBuffer() use
        // (then ::read() is allowed)
        virtual void PeekConsume (size_t consume) { (void)consume; }

        // by default read TimeOut is possible (incoming data from network,serial..)
        // children can override to false (like String::)
        virtual bool InPutCanTimeOut () { return true; }

        // (outputCanTimeOut() is defined in Print::)

    ////////////////////////
        //////////////////// extensions: Streaming Streams to Streams
        // Stream::Send*()
        //
        // Stream::Send*() uses 1-copy transfers when PeekBuffer API is
        // available, or makes a regular transfer through a temporary buffer.
        //
        // - for efficiency, Stream classes should implement PeekAPI when
        //   possible
        // - for an efficient TimeOut management, Print/Stream classes
        //   should implement {output,input}CanTimeOut()

        using oneShotMs = esp8266::PolledTimeOut::oneShotFastMs;
        static constexpr int temporaryStackBufferSize = 64;

        // ::Send*() methods:
        // - always stop before TimeOut when "no-more-input-possible-data"
        //   or "no-more-output-possible-data" condition is met
        // - always return number of transferred bytes
        // When result is 0 or less than requested maxLen, Print::getLastSend()
        // contains an error reason.

        // transfers already buffered / immediately available data (no TimeOut)
        // returns number of transferred bytes
        size_t SendAvailable (PrintClass* to) { return SendGeneric(to, -1, -1, oneShotMs::alwaysExpired); }
        size_t SendAvailable (PrintClass& to) { return SendAvailable(&to); }

        // transfers data until TimeOut
        // returns number of transferred bytes
        size_t SendAll (PrintClass* to, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendGeneric(to, -1, -1, TimeOutMs); }
        size_t SendAll (PrintClass& to, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendAll(&to, TimeOutMs); }

        // transfers data until a char is encountered (the char is swallowed but not transferred) with TimeOut
        // returns number of transferred bytes
        size_t SendUntil (PrintClass* to, const int readUntilChar, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendGeneric(to, -1, readUntilChar, TimeOutMs); }
        size_t SendUntil (PrintClass& to, const int readUntilChar, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendUntil(&to, readUntilChar, TimeOutMs); }

        // transfers data until requested size or TimeOut
        // returns number of transferred bytes
        size_t SendSize (PrintClass* to, const ssize_t maxLen, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendGeneric(to, maxLen, -1, TimeOutMs); }
        size_t SendSize (PrintClass& to, const ssize_t maxLen, const oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires) { return SendSize(&to, maxLen, TimeOutMs); }

        // remaining size (-1 by default = unknown)
        virtual ssize_t StreamRemaining () { return -1; }

        enum class Report
        {
            Success = 0,
            TimedOut,
            ReadError,
            WriteError,
            ShortOperation,
        };

        Report getLastSendReport () const { return _SendReport; }

    protected:
        size_t SendGeneric (PrintClass* to,
                            const ssize_t len = -1,
                            const int readUntilChar = -1,
                            oneShotMs::timeType TimeOutMs = oneShotMs::neverExpires /* neverExpires=>getTimeOut() */);

        size_t SendGenericPeekBuffer(PrintClass* to, const ssize_t len, const int readUntilChar, const oneShotMs::timeType TimeOutMs);
        size_t SendGenericRegularUntil(PrintClass* to, const ssize_t len, const int readUntilChar, const oneShotMs::timeType TimeOutMs);
        size_t SendGenericRegular(PrintClass* to, const ssize_t len, const oneShotMs::timeType TimeOutMs);

        void SetReport (Report report) { _SendReport = report; }

    private:

        Report _SendReport = Report::Success;

    //////////////////// end of extensions

    protected:
        long ParseInt(char skipChar); // as ParseInt() but the given skipChar is ignored
        // this allows format characters (typically commas) in values to be ignored

        float ParseFloat(char skipChar);  // as ParseFloat() but the given skipChar is ignored
};

#endif
