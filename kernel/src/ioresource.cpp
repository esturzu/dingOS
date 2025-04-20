#include "ioresource.h"
#include "ext2.h"

/* IOResource */

IOResource::~IOResource() {}

/* StandardInput */

long StandardInput::read(char* buffer, long size) {
  return SystemCallErrorCode::NOT_IMPLEMENTED;
}

long StandardInput::write(const char* buffer, long size) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

long StandardInput::seek(long loc, SeekType seek_type) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

StandardInput::~StandardInput() {}

/* StandardOutput */

long StandardOutput::read(char* buffer, long size) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

// TODO optimize character printing - instead of using printf with "%c"
// parsing overhead, add directly to final print buffer (with _putchar)
long StandardOutput::write(const char* buffer, long size) {
  for (long i = 0; i < size; i++) printf("%c", buffer[i]);
  return size;
}

long StandardOutput::seek(long loc, SeekType seek_type) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

StandardOutput::~StandardOutput() {}

/* StandardError */

long StandardError::read(char* buffer, long size) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

long StandardError::write(const char* buffer, long size) {
  for (long i = 0; i < size; i++) error_printf("%c", buffer[i]);
  return size;
}

long StandardError::seek(long loc, SeekType seek_type) {
  return SystemCallErrorCode::INVALID_OPERATION;
}

StandardError::~StandardError() {}

/* FileResource */

FileResource::FileResource() {
  data = nullptr;
  pos = 0;
}

int FileResource::open(const char* name) {
  Node* node = find_from_abs_path(name);
  if (node == nullptr) return SystemCallErrorCode::FILE_NOT_FOUND;
  file_size = node->size_in_bytes();
  data = new char[file_size];
  int result = read_file(node, data, file_size);
  if (result < 0) return SystemCallErrorCode::FILE_NOT_FOUND;
  return 0;
}

long FileResource::read(char* buffer, long size) {
  if (pos >= file_size || size <= 0) return 0;
  long c = file_size - pos, output = c < size ? c : size;
  char* pointer = data + pos;
  for (long i = 0; i < output; i++) buffer[i] = pointer[i];
  pos += output;
  return output;
}

long FileResource::write(const char* buffer, long size) {
  return SystemCallErrorCode::NOT_IMPLEMENTED;
}

long FileResource::seek(long loc, SeekType seek_type) {
  if (seek_type == SeekType::ABSOLUTE) pos = loc;
  else if (seek_type == SeekType::RELATIVE) pos += loc;
  else if (seek_type == SeekType::END) pos = file_size + loc;
  else return SystemCallErrorCode::INVALID_SEEK_TYPE;
  return pos;
}

FileResource::~FileResource() {
  if (data != nullptr) delete[] data;
}

