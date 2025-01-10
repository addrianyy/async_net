#pragma once
#include "MaskingSettings.hpp"

#include <async_net/TcpConnection.hpp>
#include <async_net/Timer.hpp>
#include <async_ws/Status.hpp>
#include <websocket/Http.hpp>

#include <cstddef>
#include <span>
#include <string>

namespace async_ws::detail {

class WebSocketConnectorImpl {
  async_net::Timer timeout;
  std::string uri;
  std::string key;

  Error handle_handshake_response(const websocket::http::Response& response,
                                  MaskingSettings& masking_settings);

 public:
  explicit WebSocketConnectorImpl(async_net::Timer timeout, std::string uri);

  struct Result {
    Error error{};
    size_t bytes_consumed{};
    bool finished{};
    MaskingSettings negotiated_masking_settings{};
  };

  Result on_data_received(std::span<const uint8_t> data);

  bool send_request(async_net::TcpConnection& connection);
};

}  // namespace async_ws::detail