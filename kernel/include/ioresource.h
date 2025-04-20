#ifndef IO_RESOURCE_H
#define IO_RESOURCE_H

#include "system_call.h"

struct IOResource {
  virtual long read(char* buffer, long size) = 0;
  virtual long write(const char* buffer, long size) = 0;
  virtual long seek(long loc, SeekType seek_type);
  virtual ~IOResource();
};

struct StandardInput : public IOResource {
  virtual long read(char* buffer, long size);
  virtual long write(const char* buffer, long size);
  virtual long seek(long loc, SeekType seek_type);
  virtual ~StandardInput();
};

struct StandardOutput : public IOResource {
  virtual long read(char* buffer, long size);
  virtual long write(const char* buffer, long size);
  virtual long seek(long loc, SeekType seek_type);
  virtual ~StandardOutput();
};

struct StandardError : public IOResource {
  virtual long read(char* buffer, long size);
  virtual long write(const char* buffer, long size);
  virtual long seek(long loc, SeekType seek_type);
  virtual ~StandardError();
};

struct FileResource : public IOResource {
  char* data;
  long pos, file_size;
  FileResource();
  int open(const char* name);
  virtual long read(char* buffer, long size);
  virtual long write(const char* buffer, long size);
  virtual long seek(long loc, SeekType seek_type);
  virtual ~FileResource();
};

#endif // IO_RESOURCE_H

