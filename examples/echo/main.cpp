#include <base/Initialization.hpp>
#include <base/Log.hpp>
#include <base/Panic.hpp>

#include <async_net/IoContext.hpp>

#include <async_ws/WebSocketClient.hpp>
#include <async_ws/WebSocketServer.hpp>

class EchoClient : public std::enable_shared_from_this<EchoClient> {
  async_ws::WebSocketClient client;
  std::string ip;

 public:
  explicit EchoClient(async_ws::WebSocketClient client)
      : client(std::move(client)), ip(this->client.peer_address().stringify()) {}

  void on_binary_message_received(std::span<const uint8_t> data) {
    log_info("{} - received binary message ({} bytes)", ip, data.size());
    (void)client.send_binary_message(data);
  }

  void on_text_message_received(std::string_view data) {
    log_info("{} - received text message ({} characters): {}", ip, data.size(), data);
    (void)client.send_text_message(data);
  }

  void on_disconnected() { log_info("{} - disconnected", ip); }
  void on_error(async_ws::Status status) { log_info("{} - error {}", ip, status.stringify()); }

  void startup(std::shared_ptr<EchoClient> self) {
    log_info("{} - connected", ip);

    client.set_on_disconnected([self] { return self->on_disconnected(); });
    client.set_on_error([self](async_ws::Status status) { return self->on_error(status); });

    client.set_on_binary_message_received(
      [self](std::span<const uint8_t> data) { return self->on_binary_message_received(data); });
    client.set_on_text_message_received(
      [self](std::string_view data) { return self->on_text_message_received(data); });
  }
};

int main() {
  base::initialize();

  const uint16_t port = 44444;

  async_net::IoContext context;
  async_ws::WebSocketServer server{context, port};

  server.set_on_listening([port] { log_info("echo server listening on {}...", port); });
  server.set_on_error([](async_ws::Status status) {
    log_info("failed to start the echo server: {}", status.stringify());
  });

  server.set_on_client_connected([](std::string_view uri, async_ws::WebSocketClient client) {
    auto echo_client = std::make_shared<EchoClient>(std::move(client));
    echo_client->startup(echo_client);
  });

  verify(context.run_until_no_work(), "run failed");
}
