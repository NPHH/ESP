/*
 FS.cpp - file system wrapper
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

#include "FS.h"
#include "FSImpl.h"

using namespace fs;

static bool sflags(const char* mode, OpenMode& om, AccessMode& am);

size_t File::Write(uint8_t c) {
    if (!_p)
        return 0;

    return _p->Write(&c, 1);
}

size_t File::Write(const uint8_t *buf, size_t Size) {
    if (!_p)
        return 0;

    return _p->Write(buf, Size);
}

int File::Available() {
    if (!_p)
        return false;

    return _p->Size() - _p->Position();
}

int File::AvailableForWrite() {
    if (!_p)
        return false;

    return _p->AvailableForWrite();
}


int File::Read() {
    if (!_p)
        return -1;

    uint8_t result;
    if (_p->Read(&result, 1) != 1) {
        return -1;
    }

    return result;
}

int File::Read(uint8_t* buf, size_t Size) {
    if (!_p)
        return 0;

    return _p->Read(buf, Size);
}

int File::Peek() {
    if (!_p)
        return -1;

    size_t curPos = _p->Position();
    int result = Read();
    Seek(curPos, SeekSet);
    return result;
}

void File::Flush() {
    if (!_p)
        return;

    _p->Flush();
}

bool File::Seek(uint32_t pos, SeekMode mode) {
    if (!_p)
        return false;

    return _p->Seek(pos, mode);
}

size_t File::Position() const {
    if (!_p)
        return 0;

    return _p->Position();
}

size_t File::Size() const {
    if (!_p)
        return 0;

    return _p->Size();
}

void File::Close() {
    if (_p) {
        _p->Close();
        _p = nullptr;
    }
}

File::operator bool() const {
    return !!_p;
}

bool File::Truncate(uint32_t Size) {
    if (!_p)
        return false;

    return _p->Truncate(Size);
}

const char* File::Name() const {
    if (!_p)
        return nullptr;

    return _p->Name();
}

const char* File::FullName() const {
    if (!_p)
        return nullptr;

    return _p->FullName();
}

bool File::IsFile() const {
    if (!_p)
        return false;

    return _p->IsFile();
}

bool File::IsDirectory() const {
    if (!_p)
        return false;

    return _p->IsDirectory();
}

void File::RewindDirectory() {
    if (!_fakeDir) {
        _fakeDir = std::make_shared<Dir>(_baseFS->OpenDir(FullName()));
    } else {
        _fakeDir->Rewind();
   }
}

File File::OpenNextFile() {
    if (!_fakeDir) {
        _fakeDir = std::make_shared<Dir>(_baseFS->OpenDir(FullName()));
    }
    _fakeDir->Next();
    return _fakeDir->OpenFile("r");
}

String File::ReadString() {
    String ret;
    ret.reserve(Size() - Position());
    uint8_t temp[256];
    int countRead;
    do {
        countRead = Read(temp, sizeof(temp));
        ret.concat((const char*)temp, countRead);
    } while (countRead > 0);
    return ret;
}

time_t File::GetLastWrite() {
    if (!_p)
        return 0;

    return _p->GetLastWrite();
}

time_t File::GetCreationTime() {
    if (!_p)
        return 0;

    return _p->GetCreationTime();
}

void File::SetTimeCallBack(time_t (*cb)(void)) {
    if (!_p)
        return;
    _p->SetTimeCallBack(cb);
    _TimeCallBack = cb;
}

File Dir::OpenFile(const char* mode) {
    if (!_impl) {
        return File();
    }

    OpenMode om;
    AccessMode am;
    if (!sflags(mode, om, am)) {
        DEBUGV("Dir::OpenFile: invalid mode `%s`\r\n", mode);
        return File();
    }

    File f(_impl->OpenFile(om, am), _baseFS);
    f.SetTimeCallBack(_TimeCallBack);
    return f;
}

String Dir::FileName() {
    if (!_impl) {
        return String();
    }

    return _impl->FileName();
}

time_t Dir::FileTime() {
    if (!_impl)
        return 0;
    return _impl->FileTime();
}

time_t Dir::FileCreationTime() {
    if (!_impl)
        return 0;
    return _impl->FileCreationTime();
}

size_t Dir::FileSize() {
    if (!_impl) {
        return 0;
    }

    return _impl->FileSize();
}

bool Dir::IsFile() const {
    if (!_impl)
        return false;

    return _impl->IsFile();
}

bool Dir::IsDirectory() const {
    if (!_impl)
        return false;

    return _impl->IsDirectory();
}

bool Dir::Next() {
    if (!_impl) {
        return false;
    }

    return _impl->Next();
}

bool Dir::Rewind() {
    if (!_impl) {
        return false;
    }

    return _impl->Rewind();
}

void Dir::SetTimeCallBack(time_t (*cb)(void)) {
    if (!_impl)
        return;
    _impl->SetTimeCallBack(cb);
    _TimeCallBack = cb;
}


bool FS::SetConfig(const FSConfig &cfg) {
    if (!_impl) {
        return false;
    }

    return _impl->SetConfig(cfg);
}

bool FS::Begin() {
    if (!_impl) {
        DEBUGV("#error: FS: no implementation");
        return false;
    }
    _impl->SetTimeCallBack(_TimeCallBack);
    bool ret = _impl->Begin();
    DEBUGV("%s\n", ret? "": "#error: FS could not start");
    return ret;
}

void FS::End() {
    if (_impl) {
        _impl->End();
    }
}

bool FS::gc() {
    if (!_impl) {
        return false;
    }
    return _impl->gc();
}

bool FS::Check() {
    if (!_impl) {
        return false;
    }
    return _impl->Check();
}

bool FS::Format() {
    if (!_impl) {
        return false;
    }
    return _impl->Format();
}

bool FS::Info(FSInfo& info){
    if (!_impl) {
        return false;
    }
    return _impl->Info(info);
}

bool FS::Info64(FSInfo64& info){
    if (!_impl) {
        return false;
    }
    return _impl->Info64(info);
}

File FS::Open(const String& path, const char* mode) {
    return Open(path.c_str(), mode);
}

File FS::Open(const char* path, const char* mode) {
    if (!_impl) {
        return File();
    }

    OpenMode om;
    AccessMode am;
    if (!sflags(mode, om, am)) {
        DEBUGV("FS::Open: invalid mode `%s`\r\n", mode);
        return File();
    }
    File f(_impl->Open(path, om, am), this);
    f.SetTimeCallBack(_TimeCallBack);
    return f;
}

bool FS::Exists(const char* path) {
    if (!_impl) {
        return false;
    }
    return _impl->Exists(path);
}

bool FS::Exists(const String& path) {
    return Exists(path.c_str());
}

Dir FS::OpenDir(const char* path) {
    if (!_impl) {
        return Dir();
    }
    DirImplPtr p = _impl->OpenDir(path);
    Dir d(p, this);
    d.SetTimeCallBack(_TimeCallBack);
    return d;
}

Dir FS::OpenDir(const String& path) {
    return OpenDir(path.c_str());
}

bool FS::Remove(const char* path) {
    if (!_impl) {
        return false;
    }
    return _impl->Remove(path);
}

bool FS::Remove(const String& path) {
    return Remove(path.c_str());
}

bool FS::rmdir(const char* path) {
    if (!_impl) {
        return false;
    }
    return _impl->rmdir(path);
}

bool FS::rmdir(const String& path) {
    return rmdir(path.c_str());
}

bool FS::mkdir(const char* path) {
    if (!_impl) {
        return false;
    }
    return _impl->mkdir(path);
}

bool FS::mkdir(const String& path) {
    return mkdir(path.c_str());
}

bool FS::Rename(const char* pathFrom, const char* pathTo) {
    if (!_impl) {
        return false;
    }
    return _impl->Rename(pathFrom, pathTo);
}

bool FS::Rename(const String& pathFrom, const String& pathTo) {
    return Rename(pathFrom.c_str(), pathTo.c_str());
}

time_t FS::GetCreationTime() {
    if (!_impl) {
        return 0;
    }
    return _impl->GetCreationTime();
}

void FS::SetTimeCallBack(time_t (*cb)(void)) {
    if (!_impl)
        return;
    _impl->SetTimeCallBack(cb);
    _TimeCallBack = cb;
}


static bool sflags(const char* mode, OpenMode& om, AccessMode& am) {
    switch (mode[0]) {
        case 'r':
            am = AM_READ;
            om = OM_DEFAULT;
            break;
        case 'w':
            am = AM_WRITE;
            om = (OpenMode) (OM_CREATE | OM_TRUNCATE);
            break;
        case 'a':
            am = AM_WRITE;
            om = (OpenMode) (OM_CREATE | OM_APPEND);
            break;
        default:
            return false;
    }
    switch(mode[1]) {
        case '+':
            am = (AccessMode) (AM_WRITE | AM_READ);
            break;
        case 0:
            break;
        default:
            return false;
    }
    return true;
}


#if defined(FS_FREESTANDING_FUNCTIONS)

/*
TODO: move these functions to public API:
*/
File Open(const char* path, const char* mode);
File Open(String& path, const char* mode);

Dir OpenDir(const char* path);
Dir OpenDir(String& path);

template<>
bool Mount<FS>(FS& fs, const char* MountPoint);
/*
*/


struct MountEntry {
    FSImplPtr fs;
    String    path;
    MountEntry* next;
};

static MountEntry* s_Mounted = nullptr;

template<>
bool Mount<FS>(FS& fs, const char* MountPoint) {
    FSImplPtr p = fs._impl;
    if (!p || !p->Mount()) {
        DEBUGV("FSImpl Mount failed\r\n");
        return false;
    }

    !make sure MountPoint has trailing '/' here

    MountEntry* entry = new MountEntry;
    entry->fs = p;
    entry->path = MountPoint;
    entry->Next = s_Mounted;
    s_Mounted = entry;
    return true;
}


/*
    iterate over MountEntries and look for the ones which match the path
*/
File Open(const char* path, const char* mode) {
    OpenMode om;
    AccessMode am;
    if (!sflags(mode, om, am)) {
        DEBUGV("Open: invalid mode `%s`\r\n", mode);
        return File();
    }

    for (MountEntry* entry = s_Mounted; entry; entry = entry->Next) {
        size_t offset = entry->path.length();
        if (strstr(path, entry->path.c_str())) {
            File result = entry->fs->Open(path + offset);
            if (result)
                return result;
        }
    }

    return File();
}

File Open(const String& path, const char* mode) {
    return FS::Open(path.c_str(), mode);
}

Dir OpenDir(const String& path) {
    return OpenDir(path.c_str());
}
#endif
