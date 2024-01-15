/*
 FSImpl.h - base File system interface
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 This File is part of the esp8266 core for Arduino environment.

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
#ifndef FSIMPL_H
#define FSIMPL_H

#include <stddef.h>
#include <stdint.h>
#include <FS.h>

namespace fs {

class FileImpl {
public:
    virtual ~FileImpl() { }
    virtual size_t Write(const uint8_t *buf, size_t size) = 0;
    virtual int Read(uint8_t* buf, size_t size) = 0;
    virtual void Flush() = 0;
    virtual bool Seek(uint32_t pos, SeekMode mode) = 0;
    virtual size_t Position() const = 0;
    virtual size_t Size() const = 0;
    virtual int AvailableForWrite() { return 0; }
    virtual bool Truncate(uint32_t size) = 0;
    virtual void Close() = 0;
    virtual const char* Name() const = 0;
    virtual const char* FullName() const = 0;
    virtual bool IsFile() const = 0;
    virtual bool IsDirectory() const = 0;

    // Filesystems *may* support a timestamp per-File, so allow the user to override with
    // their own CallBack for *this specific* File (as opposed to the FSImpl call of the
    // same name.  The default implementation simply returns time(null)
    virtual void SetTimeCallBack(time_t (*cb)(void)) { _TimeCallBack = cb; }

    // Return the last written time for a File.  Undefined when called on a writable File
    // as the FS is allowed to return either the time of the last write() operation or the
    // time present in the Filesystem metadata (often the last time the File was closed)
    virtual time_t GetLastWrite() { return 0; } // Default is to not support timestamps
    // Same for creation time.
    virtual time_t GetCreationTime() { return 0; } // Default is to not support timestamps

protected:
    time_t (*_TimeCallBack)(void) = nullptr;
};

enum OpenMode {
    OM_DEFAULT = 0,
    OM_CREATE = 1,
    OM_APPEND = 2,
    OM_TRUNCATE = 4
};

enum AccessMode {
    AM_READ = 1,
    AM_WRITE = 2,
    AM_RW = AM_READ | AM_WRITE
};

class DirImpl {
public:
    virtual ~DirImpl() { }
    virtual FileImplPtr OpenFile(OpenMode openMode, AccessMode accessMode) = 0;
    virtual const char* FileName() = 0;
    virtual size_t FileSize() = 0;
    // Return the last written time for a File.  Undefined when called on a writable File
    // as the FS is allowed to return either the time of the last write() operation or the
    // time present in the Filesystem metadata (often the last time the File was closed)
    virtual time_t FileTime() { return 0; } // By default, FS doesn't report File times
    virtual time_t FileCreationTime() { return 0; } // By default, FS doesn't report File times
    virtual bool IsFile() const = 0;
    virtual bool IsDirectory() const = 0;
    virtual bool Next() = 0;
    virtual bool Rewind() = 0;

    // Filesystems *may* support a timestamp per-File, so allow the user to override with
    // their own CallBack for *this specific* File (as opposed to the FSImpl call of the
    // same name.  The default implementation simply returns time(null)
    virtual void SetTimeCallBack(time_t (*cb)(void)) { _TimeCallBack = cb; }

protected:
    time_t (*_TimeCallBack)(void) = nullptr;
};

class FSImpl {
public:
    virtual ~FSImpl () { }
    virtual bool SetConfig(const FSConfig &cfg) = 0;
    virtual bool Begin() = 0;
    virtual void End() = 0;
    virtual bool Format() = 0;
    virtual bool Info(FSInfo& info) = 0;
    virtual bool Info64(FSInfo64& info) = 0;
    virtual FileImplPtr Open(const char* path, OpenMode openMode, AccessMode accessMode) = 0;
    virtual bool Exists(const char* path) = 0;
    virtual DirImplPtr OpenDir(const char* path) = 0;
    virtual bool Rename(const char* pathFrom, const char* pathTo) = 0;
    virtual bool Remove(const char* path) = 0;
    virtual bool mkdir(const char* path) = 0;
    virtual bool rmdir(const char* path) = 0;
    virtual bool gc() { return true; } // May not be implemented in all File systems.
    virtual bool Check() { return true; } // May not be implemented in all File systems.
    virtual time_t GetCreationTime() { return 0; } // May not be implemented in all File systems.

    // Filesystems *may* support a timestamp per-File, so allow the user to override with
    // their own CallBack for all Files on this FS.  The default implementation simply
    // returns the present time as reported by time(null)
    virtual void SetTimeCallBack(time_t (*cb)(void)) { _TimeCallBack = cb; }

protected:
    time_t (*_TimeCallBack)(void) = nullptr;
};

} // namespace fs

#endif //FSIMPL_H
