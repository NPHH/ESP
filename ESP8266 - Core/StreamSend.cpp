/*
    StreamDev.cpp - 1-copy transfer related methods
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

    parsing functions based on TextFinder library by Michael Margolis
*/

#include <Arduino.h>
#include <StreamDev.h>

size_t Stream::SendGeneric(PrintClass* to, const ssize_t len, const int readUntilChar,
                           const esp8266::PolledTimeOut::oneShotFastMs::timeType timeoutMs)
{
    SetReport(Report::Success);

    if (len == 0)
    {
        return 0;  // conveniently avoids timeout for no requested data
    }

    // There are two timeouts:
    // - read (network, serial, ...)
    // - write (network, serial, ...)
    // However
    // - GetTimeOut() is for reading only
    // - there is no getOutputTimeout() api
    // So we use GetTimeOut() for both,
    // (also when InPutCanTimeOut() is false)

    if (HasPeekBufferAPI())
    {
        return SendGenericPeekBuffer(to, len, readUntilChar, timeoutMs);
    }

    if (readUntilChar >= 0)
    {
        return SendGenericRegularUntil(to, len, readUntilChar, timeoutMs);
    }

    return SendGenericRegular(to, len, timeoutMs);
}

size_t
Stream::SendGenericPeekBuffer(PrintClass* to, const ssize_t len, const int readUntilChar,
                              const esp8266::PolledTimeOut::oneShotFastMs::timeType timeoutMs)
{
    // "neverExpires (default, impossible)" is translated to default timeout
    esp8266::PolledTimeOut::oneShotFastMs timedOut(
        timeoutMs >= esp8266::PolledTimeOut::oneShotFastMs::neverExpires ? GetTimeOut()
                                                                         : timeoutMs);
    // len==-1 => maxLen=0 <=> until starvation
    const size_t maxLen  = std::max((ssize_t)0, len);
    size_t       written = 0;

    while (!maxLen || written < maxLen)
    {
        size_t avpk = PeekAvailable();
        if (avpk == 0 && !InPutCanTimeOut())
        {
            // no more data to read, ever
            break;
        }

        size_t w = to->AvailableForWrite();
        if (w == 0 && !to->OutPutCanTimeOut())
        {
            // no more data can be written, ever
            break;
        }

        w = std::min(w, avpk);
        if (maxLen)
        {
            w = std::min(w, maxLen - written);
        }
        if (w)
        {
            const char* directbuf = PeekBuffer();
            bool        foundChar = false;
            if (readUntilChar >= 0)
            {
                const char* last = (const char*)memchr(directbuf, readUntilChar, w);
                if (last)
                {
                    w         = std::min((size_t)(last - directbuf), w);
                    foundChar = true;
                }
            }
            if (w && ((w = to->Write(directbuf, w))))
            {
                PeekConsume(w);
                written += w;
                timedOut.reset();  // something has been written
            }
            if (foundChar)
            {
                PeekConsume(1);
                break;
            }
        }

        if (timedOut)
        {
            // either (maxLen>0) nothing has been transferred for too long
            // or readUntilChar >= 0 but char is not encountered for too long
            // or (maxLen=0) too much time has been spent here
            break;
        }

        optimistic_yield(1000);
    }

    if (getLastSendReport() == Report::Success && maxLen > 0)
    {
        if (timeoutMs && timedOut)
        {
            SetReport(Report::TimedOut);
        }
        else if ((ssize_t)written != len)
        {
            // This is happening when source cannot timeout (ex: a String)
            // but has not enough data, or a dest has closed or cannot
            // timeout but is too small (String, buffer...)
            //
            // Mark it as an error because user usually wants to get what is
            // asked for.
            SetReport(Report::ShortOperation);
        }
    }

    return written;
}

size_t
Stream::SendGenericRegularUntil(PrintClass* to, const ssize_t len, const int readUntilChar,
                                const esp8266::PolledTimeOut::oneShotFastMs::timeType timeoutMs)
{
    // regular Stream API
    // no other choice than reading byte by byte

    // "neverExpires (default, impossible)" is translated to default timeout
    esp8266::PolledTimeOut::oneShotFastMs timedOut(
        timeoutMs >= esp8266::PolledTimeOut::oneShotFastMs::neverExpires ? GetTimeOut()
                                                                         : timeoutMs);
    // len==-1 => maxLen=0 <=> until starvation
    const size_t maxLen  = std::max((ssize_t)0, len);
    size_t       written = 0;

    while (!maxLen || written < maxLen)
    {
        size_t avr = Available();
        if (avr == 0 && !InPutCanTimeOut())
        {
            // no more data to read, ever
            break;
        }

        size_t w = to->AvailableForWrite();
        if (w == 0 && !to->OutPutCanTimeOut())
        {
            // no more data can be written, ever
            break;
        }

        int c = Read();
        if (c != -1)
        {
            if (c == readUntilChar)
            {
                break;
            }
            w = to->Write(c);
            if (w != 1)
            {
                SetReport(Report::WriteError);
                break;
            }
            written += 1;
            timedOut.reset();  // something has been written
        }

        if (timedOut)
        {
            // either (maxLen>0) nothing has been transferred for too long
            // or readUntilChar >= 0 but char is not encountered for too long
            // or (maxLen=0) too much time has been spent here
            break;
        }

        optimistic_yield(1000);
    }

    if (getLastSendReport() == Report::Success && maxLen > 0)
    {
        if (timeoutMs && timedOut)
        {
            SetReport(Report::TimedOut);
        }
        else if ((ssize_t)written != len)
        {
            // This is happening when source cannot timeout (ex: a String)
            // but has not enough data, or a dest has closed or cannot
            // timeout but is too small (String, buffer...)
            //
            // Mark it as an error because user usually wants to get what is
            // asked for.
            SetReport(Report::ShortOperation);
        }
    }

    return written;
}

size_t Stream::SendGenericRegular(PrintClass* to, const ssize_t len,
                                  const esp8266::PolledTimeOut::oneShotFastMs::timeType timeoutMs)
{
    // regular Stream API
    // use an intermediary buffer

    // "neverExpires (default, impossible)" is translated to default timeout
    esp8266::PolledTimeOut::oneShotFastMs timedOut(
        timeoutMs >= esp8266::PolledTimeOut::oneShotFastMs::neverExpires ? GetTimeOut()
                                                                         : timeoutMs);
    // len==-1 => maxLen=0 <=> until starvation
    const size_t maxLen  = std::max((ssize_t)0, len);
    size_t       written = 0;

    while (!maxLen || written < maxLen)
    {
        size_t avr = Available();
        if (avr == 0 && !InPutCanTimeOut())
        {
            // no more data to read, ever
            break;
        }

        size_t w = to->AvailableForWrite();
        if (w == 0 && !to->OutPutCanTimeOut())
        // no more data can be written, ever
        {
            break;
        }

        w = std::min(w, avr);
        if (maxLen)
        {
            w = std::min(w, maxLen - written);
        }
        w = std::min(w, (decltype(w))temporaryStackBufferSize);
        if (w)
        {
            char    temp[w];
            ssize_t r = Read(temp, w);
            if (r < 0)
            {
                SetReport(Report::ReadError);
                break;
            }
            w = to->Write(temp, r);
            written += w;
            if ((size_t)r != w)
            {
                SetReport(Report::WriteError);
                break;
            }
            timedOut.reset();  // something has been written
        }

        if (timedOut)
        {
            // either (maxLen>0) nothing has been transferred for too long
            // or readUntilChar >= 0 but char is not encountered for too long
            // or (maxLen=0) too much time has been spent here
            break;
        }

        optimistic_yield(1000);
    }

    if (getLastSendReport() == Report::Success && maxLen > 0)
    {
        if (timeoutMs && timedOut)
        {
            SetReport(Report::TimedOut);
        }
        else if ((ssize_t)written != len)
        {
            // This is happening when source cannot timeout (ex: a String)
            // but has not enough data, or a dest has closed or cannot
            // timeout but is too small (String, buffer...)
            //
            // Mark it as an error because user usually wants to get what is
            // asked for.
            SetReport(Report::ShortOperation);
        }
    }

    return written;
}

Stream& operator<<(Stream& out, String& string)
{
    StreamConstPtr(string).SendAll(out);
    return out;
}

Stream& operator<<(Stream& out, StreamString& stream)
{
    stream.SendAll(out);
    return out;
}

Stream& operator<<(Stream& out, Stream& stream)
{
    if (stream.StreamRemaining() < 0)
    {
        if (stream.InPutCanTimeOut())
        {
            // restrict with only what's buffered on input
            stream.SendAvailable(out);
        }
        else
        {
            // take all what is in input
            stream.SendAll(out);
        }
    }
    else
    {
        stream.SendSize(out, stream.StreamRemaining());
    }
    return out;
}

Stream& operator<<(Stream& out, const char* text)
{
    StreamConstPtr(text, strlen_P(text)).SendAll(out);
    return out;
}

Stream& operator<<(Stream& out, const __FlashStringHelper* text)
{
    StreamConstPtr(text).SendAll(out);
    return out;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_STREAMDEV)
StreamNull devnull;
#endif
