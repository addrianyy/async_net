#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace websocket::http {

enum class Method {
  Get,
  Head,
  Options,
  Trace,
  Put,
  Delete,
  Post,
  Patch,
  Connect,
};

struct Version {
  uint32_t major{};
  uint32_t minor{};

  bool is(uint32_t v_major, uint32_t v_minor) const { return major == v_major && minor == v_minor; }
  bool is_at_least(uint32_t v_major, uint32_t v_minor) const {
    return major > v_major || (major == v_major && minor >= v_minor);
  }
};

struct Header {
  std::string key;
  std::string value;
};

struct Headers {
  std::vector<Header> headers;

  void set(std::string key, std::string value);
  std::optional<std::string_view> get(std::string_view key) const;
};

struct Request {
  Method method{};
  std::string uri{};
  Version version{};

  Headers headers{};
};

struct Response {
  Version version{};
  uint32_t status{};

  Headers headers{};
};

class Serializer {
 public:
  static std::string serialize(const Request& request);
  static std::string serialize(const Response& response);
};

class Deserializer {
 public:
  enum class Error {
    Ok,
    NotEnoughData,
    TooLarge,
    MalformedFirstLine,
    InvalidMethod,
    InvalidVersion,
    InvalidStatus,
    InvalidHeaders,
  };

  template <typename T>
  struct Result {
    Error error{};
    size_t consumed_size{};
    T deserialized{};
  };

  static Result<Request> deserialize_request(std::string_view data);
  static Result<Response> deserialize_response(std::string_view data);

  static Result<Request> deserialize_request(std::span<const uint8_t> data);
  static Result<Response> deserialize_response(std::span<const uint8_t> data);
};

}  // namespace websocket::http