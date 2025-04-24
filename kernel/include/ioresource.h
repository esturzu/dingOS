#ifndef IO_RESOURCE_H
#define IO_RESOURCE_H

#include "system_call.h"

struct IOResource {
  virtual Syscall::Result<long> read(char* buffer, long size) = 0;
  virtual Syscall::Result<long> write(const char* buffer, long size) = 0;
  virtual Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type);
  virtual ~IOResource();
};

struct StandardInput : public IOResource {
  virtual Syscall::Result<long> read(char* buffer, long size);
  virtual Syscall::Result<long> write(const char* buffer, long size);
  virtual Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type);
  virtual ~StandardInput();
};

struct StandardOutput : public IOResource {
  virtual Syscall::Result<long> read(char* buffer, long size);
  virtual Syscall::Result<long> write(const char* buffer, long size);
  virtual Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type);
  virtual ~StandardOutput();
};

struct StandardError : public IOResource {
  virtual Syscall::Result<long> read(char* buffer, long size);
  virtual Syscall::Result<long> write(const char* buffer, long size);
  virtual Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type);
  virtual ~StandardError();
};

struct FileResource : public IOResource {
  char* data;
  long pos, file_size;
  FileResource();
  Syscall::ErrorCode open(const char* name);
  virtual Syscall::Result<long> read(char* buffer, long size);
  virtual Syscall::Result<long> write(const char* buffer, long size);
  virtual Syscall::Result<long> seek(long loc, Syscall::SeekType seek_type);
  virtual ~FileResource();
};

#endif // IO_RESOURCE_H

