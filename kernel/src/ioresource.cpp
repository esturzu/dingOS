#include "ioresource.h"
#include "ext2.h"
#include "uart.h"

#define RESULT_LONG Syscall::Result<long>
#define SEEK_TYPE Syscall::SeekType

/* IOResource */

IOResource::~IOResource() {}

/* StandardInput */

RESULT_LONG StandardInput::read(char* buffer, long size) {
  return Syscall::NOT_IMPLEMENTED;
}

RESULT_LONG StandardInput::write(const char* buffer, long size) {
  return Syscall::INVALID_OPERATION;
}

RESULT_LONG StandardInput::seek(long loc, SEEK_TYPE seek_type) {
  return Syscall::INVALID_OPERATION;
}

StandardInput::~StandardInput() {}

/* StandardOutput */

RESULT_LONG StandardOutput::read(char* buffer, long size) {
  return Syscall::INVALID_OPERATION;
}

RESULT_LONG StandardOutput::write(const char* buffer, long size) {
  if (size < 0) return Syscall::INVALID_SIZE;
  for (long i = 0; i < size; i++) uart_putc(buffer[i]);
  return size;
}

RESULT_LONG StandardOutput::seek(long loc, SEEK_TYPE seek_type) {
  return Syscall::INVALID_OPERATION;
}

StandardOutput::~StandardOutput() {}

/* StandardError */

RESULT_LONG StandardError::read(char* buffer, long size) {
  return Syscall::INVALID_OPERATION;
}

RESULT_LONG StandardError::write(const char* buffer, long size) {
  if (size < 0) return Syscall::INVALID_SIZE;
  for (long i = 0; i < size; i++) uart_putc(buffer[i]);
  return size;
}

RESULT_LONG StandardError::seek(long loc, SEEK_TYPE seek_type) {
  return Syscall::INVALID_OPERATION;
}

StandardError::~StandardError() {}

/* FileResource */

FileResource::FileResource() {
  data = nullptr;
  pos = 0;
}

Syscall::ErrorCode FileResource::open(const char* name) {
  Node* node = find_from_abs_path(name);
  if (node == nullptr) return Syscall::FILE_NOT_FOUND;
  file_size = node->size_in_bytes();
  data = new char[file_size];
  int result = read_file(node, data, file_size);
  if (result < 0) return Syscall::FILE_NOT_FOUND;
  return Syscall::SUCCESS;
}

RESULT_LONG FileResource::read(char* buffer, long size) {
  if (pos > file_size || pos < 0) return Syscall::INVALID_FILE_POS;
  if (size < 0) return Syscall::INVALID_SIZE;
  long c = file_size - pos, output = c < size ? c : size;
  char* pointer = data + pos;
  for (long i = 0; i < output; i++) buffer[i] = pointer[i];
  pos += output;
  return output;
}

RESULT_LONG FileResource::write(const char* buffer, long size) {
  return Syscall::NOT_IMPLEMENTED;
}

RESULT_LONG FileResource::seek(long loc, SEEK_TYPE seek_type) {
  if (seek_type == Syscall::SEEK_ABSOLUTE) pos = loc;
  else if (seek_type == Syscall::SEEK_RELATIVE) pos += loc;
  else if (seek_type == Syscall::SEEK_ENDING) pos = file_size + loc;
  else return Syscall::INVALID_SEEK_TYPE;
  return pos;
}

FileResource::~FileResource() {
  if (data != nullptr) delete[] data;
}

#undef RESULT_LONG
#undef SEEK_TYPE

