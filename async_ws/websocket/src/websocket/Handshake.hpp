#pragma once
#include "Http.hpp"

namespace websocket::handshake {

enum class Error {
  Ok,
  InvalidMethod,
  InvalidVersion,
  InvalidHeaders,
  InvalidKey,
  InvalidWebSocketVersion,
  InvalidProtocol,
  UnexpectedBody,
  UpgradeRejected,
};

namespace client {

std::string generate_random_key();
http::Request create_http_request(std::string_view uri, std::string_view key);
Error validate_http_response(const http::Response& response, std::string_view request_key);

}  // namespace client

namespace server {

Error validate_http_request(const http::Request& request);
http::Response respond_to_http_request(const http::Request& request);
http::Response respond_error_to_http_request(const http::Request& request, uint32_t status);

}  // namespace server

}  // namespace websocket::handshake