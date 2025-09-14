#pragma once
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include <base/macro/EnumBitOperations.hpp>

namespace base {

class File {
  std::FILE* fp = nullptr;

 public:
  enum class OpenFlags {
    None = 0,
    NoBuffering = (1 << 0),
  };

  enum class SeekOrigin {
    Set,
    Current,
    End,
  };

  static bool read_text_file(const std::string& path, std::string& output);
  static bool read_binary_file(const std::string& path, std::vector<uint8_t>& output);

  static bool write_text_file(const std::string& path, std::string_view contents);
  static bool write_binary_file(const std::string& path, const void* data, size_t size);

  File() = default;
  File(const std::string& path, const char* mode, OpenFlags flags = OpenFlags::None);

  ~File();

  File(File&& other) noexcept;
  File& operator=(File&& other) noexcept;

  File(File&) = delete;
  File& operator=(File&) = delete;

  std::FILE* handle();

  operator bool() const { return opened(); }

  bool opened() const;
  bool error() const;
  bool eof() const;

  int64_t tell() const;

  void seek(SeekOrigin origin, int64_t offset);
  void flush();

  size_t read(void* buffer, size_t size);
  size_t read(std::span<uint8_t> buffer) { return read(buffer.data(), buffer.size()); }

  size_t write(const void* buffer, size_t size);
  size_t write(std::span<const uint8_t> buffer) { return write(buffer.data(), buffer.size()); }

  void close();
};

}  // namespace base

IMPLEMENT_ENUM_BIT_OPERATIONS(base::File::OpenFlags)
