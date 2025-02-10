#define _CRT_SECURE_NO_WARNINGS

#include "File.hpp"

#include <base/Platform.hpp>

#include <cstdio>

using namespace base;

template <typename T>
static bool read_file_internal(const std::string& path, const char* mode, T& output) {
  output.clear();

  File file(path, mode, File::OpenFlags::NoBuffering);
  if (!file) {
    return false;
  }

  file.seek(File::SeekOrigin::End, 0);
  const auto file_size = file.tell();
  file.seek(File::SeekOrigin::Set, 0);

  output.resize(file_size + 1);

  const auto read_size = file.read(output.data(), output.size());
  if (file.error() || read_size > file_size) {
    output.clear();
    return false;
  }

  output.resize(read_size);

  return true;
}

static bool write_file_internal(const std::string& path,
                                const char* mode,
                                const void* buffer,
                                size_t buffer_size) {
  File file(path, mode, File::OpenFlags::NoBuffering);
  if (!file) {
    return false;
  }

  const auto size = file.write(buffer, buffer_size);
  if (size != buffer_size) {
    return false;
  }

  return true;
}

bool File::read_binary_file(const std::string& path, std::vector<uint8_t>& output) {
  return read_file_internal(path, "rb", output);
}

bool File::read_text_file(const std::string& path, std::string& output) {
  return read_file_internal(path, "r", output);
}

bool File::write_binary_file(const std::string& path, const void* data, size_t size) {
  return write_file_internal(path, "wb", data, size);
}

bool File::write_text_file(const std::string& path, std::string_view contents) {
  return write_file_internal(path, "w", contents.data(), contents.size());
}

File::File(const std::string& path, const char* mode, OpenFlags flags) {
  fp = std::fopen(path.c_str(), mode);

  if ((flags & OpenFlags::NoBuffering) == OpenFlags::NoBuffering) {
    if (fp) {
      std::setbuf(fp, nullptr);
    }
  }
}

File::~File() {
  close();
}

File::File(File&& other) noexcept {
  fp = other.fp;
  other.fp = nullptr;
}

File& File::operator=(File&& other) noexcept {
  if (this != &other) {
    if (fp) {
      std::fclose(fp);
    }
    fp = other.fp;
    other.fp = nullptr;
  }

  return *this;
}

std::FILE* File::handle() {
  return fp;
}

bool File::opened() const {
  return fp != nullptr;
}

bool File::error() const {
  return std::ferror(fp) != 0;
}

bool File::eof() const {
  return std::feof(fp) != 0;
}

int64_t File::tell() const {
#ifdef PLATFORM_WINDOWS
  return _ftelli64(fp);
#else
  return std::ftell(fp);
#endif
}

void File::seek(SeekOrigin origin, int64_t offset) {
  int c_origin;
  switch (origin) {
    case SeekOrigin::Current:
      c_origin = SEEK_CUR;
      break;

    case SeekOrigin::Set:
      c_origin = SEEK_SET;
      break;

    case SeekOrigin::End:
      c_origin = SEEK_END;
      break;
  }

#ifdef PLATFORM_WINDOWS
  _fseeki64(fp, offset, c_origin);
#else
  std::fseek(fp, offset, c_origin);
#endif
}

void File::flush() {
  std::fflush(fp);
}

size_t File::read(void* buffer, size_t size) {
  return std::fread(buffer, 1, size, fp);
}

size_t File::write(const void* buffer, size_t size) {
  return std::fwrite(buffer, 1, size, fp);
}

void File::close() {
  if (fp) {
    std::fclose(fp);
    fp = nullptr;
  }
}
