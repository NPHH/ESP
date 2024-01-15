/* 
 cbuf.cpp - Circular buffer implementation
 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
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

#include <new> // std::nothrow
#include "cbuf.h"
#include "c_types.h"

cbuf::cbuf(size_t size) :
    next(NULL), _size(size), _buf(new char[size]), _bufend(_buf + size), _Begin(_buf), _end(_Begin) {
}

cbuf::~cbuf() {
    delete[] _buf;
}

size_t cbuf::resizeAdd(size_t addSize) {
    return resize(_size + addSize);
}

size_t cbuf::resize(size_t newSize) {

    size_t bytes_available = available();

	// not lose any data
	// if data can be lost use remove or flush before resize
    if((newSize <= bytes_available) || (newSize == _size)) {
        return _size;
    }

    char *newbuf = new (std::nothrow) char[newSize];
    char *oldbuf = _buf;

    if(!newbuf) {
        return _size;
    }

    if(_buf) {
        read(newbuf, bytes_available);
        memset((newbuf + bytes_available), 0x00, (newSize - bytes_available));
    }

    _Begin = newbuf;
    _end = newbuf + bytes_available;
    _bufend = newbuf + newSize;
    _size = newSize;

    _buf = newbuf;
    delete[] oldbuf;

    return _size;
}

size_t IRAM_ATTR cbuf::available() const {
    if(_end >= _Begin) {
        return _end - _Begin;
    }
    return _size - (_Begin - _end);
}

size_t cbuf::size() {
    return _size;
}

size_t cbuf::room() const {
    if(_end >= _Begin) {
        return _size - (_end - _Begin) - 1;
    }
    return _Begin - _end - 1;
}

int cbuf::peek() {
    if(empty())
        return -1;

    return static_cast<int>(*_Begin);
}

size_t cbuf::peek(char *dst, size_t size) {
    size_t bytes_available = available();
    size_t size_to_read = (size < bytes_available) ? size : bytes_available;
    size_t size_read = size_to_read;
    char * Begin = _Begin;
    if(_end < _Begin && size_to_read > (size_t) (_bufend - _Begin)) {
        size_t top_size = _bufend - _Begin;
        memcpy(dst, _Begin, top_size);
        Begin = _buf;
        size_to_read -= top_size;
        dst += top_size;
    }
    memcpy(dst, Begin, size_to_read);
    return size_read;
}

int IRAM_ATTR cbuf::read() {
    if(empty())
        return -1;

    char result = *_Begin;
    _Begin = wrap_if_bufend(_Begin + 1);
    return static_cast<int>(result);
}

size_t cbuf::read(char* dst, size_t size) {
    size_t bytes_available = available();
    size_t size_to_read = (size < bytes_available) ? size : bytes_available;
    size_t size_read = size_to_read;
    if(_end < _Begin && size_to_read > (size_t) (_bufend - _Begin)) {
        size_t top_size = _bufend - _Begin;
        memcpy(dst, _Begin, top_size);
        _Begin = _buf;
        size_to_read -= top_size;
        dst += top_size;
    }
    memcpy(dst, _Begin, size_to_read);
    _Begin = wrap_if_bufend(_Begin + size_to_read);
    return size_read;
}

size_t IRAM_ATTR cbuf::write(char c) {
    if(full())
        return 0;

    *_end = c;
    _end = wrap_if_bufend(_end + 1);
    return 1;
}

size_t cbuf::write(const char* src, size_t size) {
    size_t bytes_available = room();
    size_t size_to_write = (size < bytes_available) ? size : bytes_available;
    size_t size_written = size_to_write;
    if(_end >= _Begin && size_to_write > (size_t) (_bufend - _end)) {
        size_t top_size = _bufend - _end;
        memcpy(_end, src, top_size);
        _end = _buf;
        size_to_write -= top_size;
        src += top_size;
    }
    memcpy(_end, src, size_to_write);
    _end = wrap_if_bufend(_end + size_to_write);
    return size_written;
}

void cbuf::flush() {
    _Begin = _buf;
    _end = _buf;
}

size_t cbuf::remove(size_t size) {
    size_t bytes_available = available();
    if(size >= bytes_available) {
        flush();
        return 0;
    }
    size_t size_to_remove = (size < bytes_available) ? size : bytes_available;
    if(_end < _Begin && size_to_remove > (size_t) (_bufend - _Begin)) {
        size_t top_size = _bufend - _Begin;
        _Begin = _buf;
        size_to_remove -= top_size;
    }
    _Begin = wrap_if_bufend(_Begin + size_to_remove);
    return available();
}
