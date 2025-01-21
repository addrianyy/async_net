#include "WebSocketAcceptingClientImpl.hpp"
#include "ConnectionTimeout.hpp"
#include "CustomMaskingOverride.hpp"
#include "MaskingSettings.hpp"
#include "WebSocketServerImpl.hpp"

#include <async_ws/WebSocketClient.hpp>

#include <websocket/Handshake.hpp>

#include <base/Log.hpp>

namespace async_ws::detail {

bool WebSocketAcceptingClientImpl::handle_handshake_request(
  const websocket::http::Request& request) {
  MaskingSettings masking_settings{
    .send_masked = false,
    .receive_masked = true,
  };

  const bool has_masking_override =
    CustomMaskingOverride::contain_masking_override(request.headers);
  if (has_masking_override) {
    masking_settings.send_masked = false;
    masking_settings.receive_masked = false;
  }

  const auto reject = [&] {
    (void)connection.send([&](base::BinaryBuffer& buffer) {
      const auto response = websocket::http::Serializer::serialize(
        websocket::handshake::server::respond_error_to_http_request(request, 401));
      buffer.append(response.data(), response.size());
    });
  };

  const auto validate_result = websocket::handshake::server::validate_http_request(request);
  if (validate_result != websocket::handshake::Error::Ok) {
    reject();
    log_warn("accepting WebSocket client: received invalid HTTP request");
    return false;
  }

  if (auto serverS = server.lock()) {
    if (serverS->on_connection_request) {
      if (!serverS->on_connection_request(request.uri, connection.peer_address())) {
        reject();
        return false;
      }
    }

    if (!serverS->on_client_connected || serverS->state() != WebSocketServer::State::Listening) {
      reject();
      return false;
    }

    const auto send_result = connection.send([&](base::BinaryBuffer& buffer) {
      auto response = websocket::handshake::server::respond_to_http_request(request);
      if (has_masking_override) {
        CustomMaskingOverride::inject_masking_override(response.headers);
      }

      const auto serialized = websocket::http::Serializer::serialize(response);
      buffer.append(serialized.data(), serialized.size());
    });
    if (!send_result) {
      return false;
    }

    // Clear the callbacks.
    connection.set_on_disconnected(nullptr);
    connection.set_on_error(nullptr);
    connection.set_on_data_received(nullptr);

    WebSocketClient client{std::move(connection), masking_settings};
    serverS->on_client_connected(request.uri, std::move(client));

    return true;
  }

  return false;
}

size_t WebSocketAcceptingClientImpl::on_data_received(std::span<const uint8_t> data) {
  const auto result = websocket::http::Deserializer::deserialize_request(data);

  if (result.error == websocket::http::Deserializer::Error::NotEnoughData) {
    return 0;
  }

  // At this point we will either fail or succeed.
  timeout.reset();

  if (result.error == websocket::http::Deserializer::Error::Ok) {
    if (result.consumed_size == data.size()) {
      if (handle_handshake_request(result.deserialized)) {
        return result.consumed_size;
      }
    } else {
      log_warn("accepting WebSocket client: received more data than exepcted");
    }
  } else {
    log_warn("accepting WebSocket client: failed to parse HTTP request");
  }

  connection.shutdown();

  return data.size();
}

WebSocketAcceptingClientImpl::WebSocketAcceptingClientImpl(
  std::weak_ptr<WebSocketServerImpl> server,
  async_net::TcpConnection connection)
    : io_context(*connection.io_context()),
      server(std::move(server)),
      connection(std::move(connection)) {}

void WebSocketAcceptingClientImpl::startup(std::shared_ptr<WebSocketAcceptingClientImpl> self) {
  std::weak_ptr<WebSocketAcceptingClientImpl> selfW = self;

  timeout = async_net::Timer::invoke_after(io_context, connection_timeout, [selfW] {
    if (auto selfS = selfW.lock()) {
      log_warn("accepting WebSocket client: timed out");
      selfS->connection.shutdown();
    }
  });

  connection.set_on_data_received(
    [self](std::span<const uint8_t> data) { return self->on_data_received(data); });

  connection.set_on_disconnected([selfW]() {
    if (auto selfS = selfW.lock()) {
      log_warn("accepting WebSocket client: disconnected");
      selfS->timeout.reset();
    }
  });

  connection.set_on_error([selfW](async_net::Status status) {
    if (auto selfS = selfW.lock()) {
      log_warn("accepting WebSocket client: error {}", status.stringify());
      selfS->timeout.reset();
    }
  });
}

}  // namespace async_ws::detail