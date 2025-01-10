#include "Handshake.hpp"
#include "detail/Sha1.hpp"

#include <base/Panic.hpp>
#include <base/text/Base64.hpp>
#include <base/text/Helpers.hpp>

#include <base/rng/Xorshift.hpp>

namespace websocket::handshake {

static std::string derive_accept_from_key(std::string_view key) {
  static constexpr std::string_view static_key = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  Sha1 sha;
  sha.feed(key.data(), key.size());
  sha.feed(static_key.data(), static_key.size());

  const auto digest = sha.digest();
  const auto accept_key = base::Base64::encode(digest);

  return accept_key;
}

namespace client {

std::string generate_random_key() {
  base::Xorshift xorshift{base::SeedFromSystemRng{}};

  const uint64_t values[]{xorshift.gen(), xorshift.gen()};
  return base::Base64::encode(std::span{reinterpret_cast<const uint8_t*>(&values), sizeof(values)});
}

http::Request create_http_request(std::string_view uri, std::string_view key) {
  http::Request request;

  request.method = http::Method::Get;
  request.uri = std::string(uri);
  request.version = http::Version{1, 1};

  request.headers.set("Upgrade", "websocket");
  request.headers.set("Connection", "upgrade");
  request.headers.set("Sec-WebSocket-Version", "13");
  request.headers.set("Sec-WebSocket-Key", std::string(key));

  return request;
}

Error validate_http_response(const http::Response& response, std::string_view request_key) {
  if (!response.version.is_at_least(1, 1)) {
    return Error::InvalidVersion;
  }

  if (response.status != 101) {
    return Error::UpgradeRejected;
  }

  const auto has_header = [&](std::string_view key, std::string_view value) {
    if (const auto header_value = response.headers.get(key)) {
      return base::text::equals_case_insensitive(*header_value, value);
    }
    return false;
  };

  if (!has_header("Upgrade", "websocket")) {
    return Error::InvalidHeaders;
  }

  if (!has_header("Connection", "Upgrade")) {
    return Error::InvalidHeaders;
  }

  if (response.headers.get("Sec-WebSocket-Protocol")) {
    return Error::InvalidProtocol;
  }

  if (response.headers.get("Content-Length")) {
    return Error::UnexpectedBody;
  }

  const auto accept = response.headers.get("Sec-WebSocket-Accept");
  if (!accept) {
    return Error::InvalidHeaders;
  }
  if (*accept != derive_accept_from_key(request_key)) {
    return Error::InvalidKey;
  }

  return Error::Ok;
}

}  // namespace client

namespace server {

Error validate_http_request(const http::Request& request) {
  if (request.method != http::Method::Get) {
    return Error::InvalidMethod;
  }

  if (!request.version.is_at_least(1, 1)) {
    return Error::InvalidVersion;
  }

  const auto has_header = [&](std::string_view key, std::string_view value) {
    if (const auto header_value = request.headers.get(key)) {
      return base::text::equals_case_insensitive(*header_value, value);
    }
    return false;
  };

  if (!has_header("Upgrade", "websocket")) {
    return Error::InvalidHeaders;
  }

  if (!has_header("Connection", "upgrade")) {
    return Error::InvalidHeaders;
  }

  if (!has_header("Sec-WebSocket-Version", "13")) {
    return Error::InvalidWebSocketVersion;
  }

  if (request.headers.get("Sec-WebSocket-Protocol")) {
    return Error::InvalidProtocol;
  }

  if (request.headers.get("Content-Length")) {
    return Error::UnexpectedBody;
  }

  const auto key = request.headers.get("Sec-WebSocket-Key");
  if (!key || key->empty()) {
    return Error::InvalidKey;
  }

  return Error::Ok;
}

http::Response respond_to_http_request(const http::Request& request) {
  const auto key = request.headers.get("Sec-WebSocket-Key");
  verify(key, "request doesn't have Sec-WebSocket-Key");

  http::Response response;

  response.version = request.version;
  response.status = 101;

  response.headers.set("Upgrade", "websocket");
  response.headers.set("Connection", "Upgrade");
  response.headers.set("Sec-WebSocket-Accept", derive_accept_from_key(*key));

  return response;
}

http::Response respond_error_to_http_request(const http::Request& request, uint32_t status) {
  http::Response response;

  response.version = request.version;
  response.status = status;

  return response;
}

}  // namespace server

}  // namespace websocket::handshake
