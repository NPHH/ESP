/*
 FS.h - file system wrapper
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
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

#ifndef FS_H
#define FS_H

#include <memory>
#include <Arduino.h>
#include <../include/time.h> // See issue #6714

class SDClass;

namespace fs {

class File;
class Dir;
class FS;

class FileImpl;
typedef std::shared_ptr<FileImpl> FileImplPtr;
class FSImpl;
typedef std::shared_ptr<FSImpl> FSImplPtr;
class DirImpl;
typedef std::shared_ptr<DirImpl> DirImplPtr;

template <typename Tfs>
bool Mount(Tfs& fs, const char* mountPoint);

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File : public Stream
{
public:
    File(FileImplPtr p = FileImplPtr(), FS *baseFS = nullptr) : _p(p), _fakeDir(nullptr), _baseFS(baseFS) { }

    // Print methods:
    size_t Write(uint8_t) override;
    size_t Write(const uint8_t *buf, size_t size) override;
    int AvailableForWrite() override;

    // Stream methods:
    int Available() override;
    int Read() override;
    int Peek() override;
    void Flush() override;
    size_t ReadBytes(char *buffer, size_t length) override {
        return Read((uint8_t*)buffer, length);
    }
    int Read(uint8_t* buf, size_t size) override;
    bool Seek(uint32_t pos, SeekMode mode);
    bool Seek(uint32_t pos) {
        return Seek(pos, SeekSet);
    }
    size_t Position() const;
    size_t Size() const;
    virtual ssize_t StreamRemaining() override { return (ssize_t)Size() - (ssize_t)Position(); }
    void Close();
    operator bool() const;
    const char* Name() const;
    const char* FullName() const; // Includes path
    bool Truncate(uint32_t size);

    bool IsFile() const;
    bool IsDirectory() const;

    // Arduino "class SD" methods for compatibility
    //TODO use stream::send / check read(buf,size) result
    template<typename T> size_t Write(T &src){
      uint8_t obuf[256];
      size_t doneLen = 0;
      size_t sentLen;

      while (src.Available() > (int)sizeof(obuf)){
        src.read(obuf, sizeof(obuf));
        sentLen = Write(obuf, sizeof(obuf));
        doneLen = doneLen + sentLen;
        if(sentLen != sizeof(obuf)){
          return doneLen;
        }
      }

      size_t leftLen = src.Available();
      src.Read(obuf, leftLen);
      sentLen = Write(obuf, leftLen);
      doneLen = doneLen + sentLen;
      return doneLen;
    }
    using PrintClass::Write;

    void RewindDirectory();
    File OpenNextFile();

    String ReadString() override;

    time_t GetLastWrite();
    time_t GetCreationTime();
    void SetTimeCallBack(time_t (*cb)(void));

    // Stream::send configuration

    bool InPutCanTimeOut () override {
        // unAvailable data can't become later Available
        return false;
    }

    bool OutPutCanTimeOut () override {
        // free space for write can't increase later
        return false;
    }

protected:
    FileImplPtr _p;
    time_t (*_TimeCallBack)(void) = nullptr;

    // Arduino SD class emulation
    std::shared_ptr<Dir> _fakeDir;
    FS                  *_baseFS;
};

class Dir {
public:
    Dir(DirImplPtr impl = DirImplPtr(), FS *baseFS = nullptr): _impl(impl), _baseFS(baseFS) { }

    File OpenFile(const char* mode);

    String FileName();
    size_t FileSize();
    time_t FileTime();
    time_t FileCreationTime();
    bool IsFile() const;
    bool IsDirectory() const;

    bool Next();
    bool Rewind();

    void SetTimeCallBack(time_t (*cb)(void));

protected:
    DirImplPtr _impl;
    FS       *_baseFS;
    time_t (*_TimeCallBack)(void) = nullptr;
};

// Backwards compatible, <4GB filesystem usage
struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};

// Support > 4GB filesystems (SD, etc.)
struct FSInfo64 {
    uint64_t totalBytes;
    uint64_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};


class FSConfig
{
public:
    static constexpr uint32_t FSId = 0x00000000;

    FSConfig(uint32_t type = FSId, bool autoFormat = true) : _type(type), _autoFormat(autoFormat) { }

    FSConfig SetAutoFormat(bool val = true) {
        _autoFormat = val;
        return *this;
    }

    uint32_t _type;
    bool     _autoFormat;
};

class SPIFFSConfig : public FSConfig
{
public:
    static constexpr uint32_t FSId = 0x53504946;
    SPIFFSConfig(bool autoFormat = true) : FSConfig(FSId, autoFormat) { }

    // Inherit _type and _autoFormat
    // nothing yet, enableTime TBD when SPIFFS has metadate
};

class FS
{
public:
    FS(FSImplPtr impl) : _impl(impl) { _TimeCallBack = _defaultTimeCB; }

    bool SetConfig(const FSConfig &cfg);

    bool Begin();
    void End();

    bool Format();
    bool Info(FSInfo& info);
    bool Info64(FSInfo64& info);

    File Open(const char* path, const char* mode);
    File Open(const String& path, const char* mode);

    bool Exists(const char* path);
    bool Exists(const String& path);

    Dir OpenDir(const char* path);
    Dir OpenDir(const String& path);

    bool Remove(const char* path);
    bool Remove(const String& path);

    bool Rename(const char* pathFrom, const char* pathTo);
    bool Rename(const String& pathFrom, const String& pathTo);

    bool mkdir(const char* path);
    bool mkdir(const String& path);

    bool rmdir(const char* path);
    bool rmdir(const String& path);

    // Low-level FS routines, not needed by most applications
    bool gc();
    bool Check();

    time_t GetCreationTime();

    void SetTimeCallBack(time_t (*cb)(void));

    friend class ::SDClass; // More of a frenemy, but SD needs internal implementation to get private FAT bits
protected:
    FSImplPtr _impl;
    FSImplPtr getImpl() { return _impl; }
    time_t (*_TimeCallBack)(void) = nullptr;
    static time_t _defaultTimeCB(void) { return time(NULL); }
};

} // namespace fs

extern "C"
{
void close_all_fs(void);
void littlefs_request_end(void);
void spiffs_request_end(void);
}

#ifndef FS_NO_GLOBALS
using fs::FS;
using fs::File;
using fs::Dir;
using fs::SeekMode;
using fs::SeekSet;
using fs::SeekCur;
using fs::SeekEnd;
using fs::FSInfo;
using fs::FSConfig;
using fs::SPIFFSConfig;
#endif //FS_NO_GLOBALS

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SPIFFS)
extern fs::FS SPIFFS __attribute__((deprecated("SPIFFS has been deprecated. Please consider moving to LittleFS or other filesystems.")));
#endif

#endif //FS_H
