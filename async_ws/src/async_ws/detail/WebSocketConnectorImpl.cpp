#include "WebSocketConnectorImpl.hpp"
#include "CustomMaskingOverride.hpp"
#include "WebSocketClientImpl.hpp"

#include <websocket/Handshake.hpp>

namespace async_ws::detail {

Error WebSocketConnectorImpl::handle_handshake_response(const websocket::http::Response& response,
                                                        MaskingSettings& masking_settings) {
  const auto error = websocket::handshake::client::validate_http_response(response, key);
  if (error != websocket::handshake::Error::Ok) {
    return error == websocket::handshake::Error::UpgradeRejected ? Error::WebSocketUpgradeRejected
                                                                 : Error::InvalidHttpResponse;
  }

  masking_settings = {
    .send_masked = true,
    .receive_masked = false,
  };
  if (CustomMaskingOverride::contain_masking_override(response.headers)) {
    masking_settings.send_masked = false;
    masking_settings.receive_masked = false;
  }

  return Error::Ok;
}

WebSocketConnectorImpl::WebSocketConnectorImpl(async_net::Timer timeout, std::string uri)
    : timeout(std::move(timeout)),
      uri(std::move(uri)),
      key(websocket::handshake::client::generate_random_key()) {}

WebSocketConnectorImpl::Result WebSocketConnectorImpl::on_data_received(
  std::span<const uint8_t> data) {
  const auto result = websocket::http::Deserializer::deserialize_response(data);

  if (result.error == websocket::http::Deserializer::Error::NotEnoughData) {
    return {.error = Error::Ok, .bytes_consumed = 0};
  }

  if (result.error == websocket::http::Deserializer::Error::Ok) {
    MaskingSettings masking_settings{};
    const auto error = handle_handshake_response(result.deserialized, masking_settings);
    if (error != Error::Ok) {
      return {.error = error, .bytes_consumed = result.consumed_size};
    }

    return {
      .error = Error::Ok,
      .bytes_consumed = result.consumed_size,
      .finished = true,
      .negotiated_masking_settings = masking_settings,
    };
  }

  return {.error = Error::InvalidHttpResponse, .bytes_consumed = 0};
}

bool WebSocketConnectorImpl::send_request(async_net::TcpConnection& connection) {
  auto request = websocket::handshake::client::create_http_request(uri, key);
  CustomMaskingOverride::inject_masking_override(request.headers);

  const auto serialized = websocket::http::Serializer::serialize(request);

  return connection.send_data(
    std::span{reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size()});
}

}  // namespace async_ws::detail