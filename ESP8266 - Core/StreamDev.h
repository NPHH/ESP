/*
    StreamDev.h - Stream helpers
    Copyright (c) 2019 David Gauchard.  All right reserved.

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
*/

#ifndef __STREAMDEV_H
#define __STREAMDEV_H

#include <limits>
#include <esp_priv.h>
#include <StreamString.h>

///////////////////////////////////////////////
// /dev/null
// - black hole as output, swallow everything, AvailableForWrite = infinite
// - black hole as input, nothing to read, Available = 0

class StreamNull: public Stream
{
public:

    // Print
    virtual size_t Write(uint8_t) override
    {
        return 1;
    }

    virtual size_t Write(const uint8_t* buffer, size_t size) override
    {
        (void)buffer;
        (void)size;
        return size;
    }

    virtual int AvailableForWrite() override
    {
        return std::numeric_limits<int16_t>::max();
    }

    // Stream
    virtual int Available() override
    {
        return 0;
    }

    virtual int Read() override
    {
        return -1;
    }

    virtual int Peek() override
    {
        return -1;
    }

    virtual size_t ReadBytes(char* buffer, size_t len) override
    {
        (void)buffer;
        (void)len;
        return 0;
    }

    virtual int Read(uint8_t* buffer, size_t len) override
    {
        (void)buffer;
        (void)len;
        return 0;
    }

    virtual bool OutPutCanTimeOut() override
    {
        return false;
    }

    virtual bool InPutCanTimeOut() override
    {
        return false;
    }

    virtual ssize_t StreamRemaining() override
    {
        return 0;
    }
};

///////////////////////////////////////////////
// /dev/zero
// - black hole as output, swallow everything, AvailableForWrite = infinite
// - big bang as input, gives infinity to read, Available = infinite

class StreamZero: public StreamNull
{
protected:

    char _zero;

public:

    StreamZero(char zero = 0): _zero(zero) { }

    // Stream
    virtual int Available() override
    {
        return std::numeric_limits<int16_t>::max();
    }

    virtual int Read() override
    {
        return _zero;
    }

    virtual int Peek() override
    {
        return _zero;
    }

    virtual size_t ReadBytes(char* buffer, size_t len) override
    {
        memset(buffer, _zero, len);
        return len;
    }

    virtual int Read(uint8_t* buffer, size_t len) override
    {
        memset((char*)buffer, _zero, len);
        return len;
    }

    virtual ssize_t StreamRemaining() override
    {
        return std::numeric_limits<int16_t>::max();
    }
};

///////////////////////////////////////////////
// static buffer (in flash or ram)
// - black hole as output, swallow everything, AvailableForWrite = infinite
// - Stream buffer out as input, resettable

class StreamConstPtr: public StreamNull
{
protected:
    const char* _buffer;
    size_t _size;
    bool _byteAddressable;
    size_t _peekPointer = 0;

public:
    StreamConstPtr(const String&& string) = delete; // prevents passing String temporary, use ctor(buffer,size) if you know what you are doing
    StreamConstPtr(const String& string): _buffer(string.c_str()), _size(string.length()), _byteAddressable(true) { }
    StreamConstPtr(const char* buffer, size_t size): _buffer(buffer), _size(size), _byteAddressable(__byteAddressable(buffer)) { }
    StreamConstPtr(const uint8_t* buffer, size_t size): _buffer((const char*)buffer), _size(size), _byteAddressable(__byteAddressable(buffer)) { }
    StreamConstPtr(const __FlashStringHelper* buffer, size_t size): _buffer(reinterpret_cast<const char*>(buffer)), _size(size), _byteAddressable(false) { }
    StreamConstPtr(const __FlashStringHelper* text): _buffer(reinterpret_cast<const char*>(text)), _size(strlen_P((PGM_P)text)), _byteAddressable(false) { }

    void resetPointer(int pointer = 0)
    {
        _peekPointer = pointer;
    }

    // Stream
    virtual int Available() override
    {
        return PeekAvailable();
    }

    virtual int Read() override
    {
        // valid with dram, iram and flash
        return _peekPointer < _size ? pgm_read_byte(&_buffer[_peekPointer++]) : -1;
    }

    virtual int Peek() override
    {
        // valid with dram, iram and flash
        return _peekPointer < _size ? pgm_read_byte(&_buffer[_peekPointer]) : -1;
    }

    virtual size_t ReadBytes(char* buffer, size_t len) override
    {
        if (_peekPointer >= _size)
        {
            return 0;
        }
        size_t cpylen = std::min(_size - _peekPointer, len);
        memcpy_P(buffer, _buffer + _peekPointer, cpylen); // whether byte adressible is true
        _peekPointer += cpylen;
        return cpylen;
    }

    virtual int Read(uint8_t* buffer, size_t len) override
    {
        return ReadBytes((char*)buffer, len);
    }

    virtual ssize_t StreamRemaining() override
    {
        return _size;
    }

    // PeekBuffer
    virtual bool HasPeekBufferAPI() const override
    {
        return _byteAddressable;
    }

    virtual size_t PeekAvailable() override
    {
        return _peekPointer < _size ? _size - _peekPointer : 0;
    }

    virtual const char* PeekBuffer() override
    {
        return _peekPointer < _size ? _buffer + _peekPointer : nullptr;
    }

    virtual void PeekConsume(size_t consume) override
    {
        _peekPointer += consume;
    }
};

///////////////////////////////////////////////

Stream& operator << (Stream& out, String& string);
Stream& operator << (Stream& out, Stream& stream);
Stream& operator << (Stream& out, StreamString& stream);
Stream& operator << (Stream& out, const char* text);
Stream& operator << (Stream& out, const __FlashStringHelper* text);

///////////////////////////////////////////////

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_STREAMDEV)
extern StreamNull devnull;
#endif

#endif // __STREAMDEV_H
