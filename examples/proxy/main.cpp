#include <base/Initialization.hpp>
#include <base/Log.hpp>
#include <base/Panic.hpp>
#include <base/text/Text.hpp>

#include <async_net/IoContext.hpp>
#include <async_net/TcpConnection.hpp>
#include <async_net/Timer.hpp>

#include <async_ws/WebSocketClient.hpp>
#include <async_ws/WebSocketServer.hpp>

class ProxyClient : public std::enable_shared_from_this<ProxyClient> {
  async_ws::WebSocketClient client;
  std::string ip;

  async_net::TcpConnection tcp_peer;

  async_net::Timer timeout;

  enum class State {
    WaitingForAddress,
    Connecting,
    Proxying,
    Closed,
  };
  State state{State::WaitingForAddress};

  void close() {
    state = State::Closed;
    client.shutdown();
    tcp_peer.shutdown();
    timeout.reset();
  }

  void send_tcp_peer_data(std::string_view data) {
    send_tcp_peer_data(std::span{reinterpret_cast<const uint8_t*>(data.data()), data.size()});
  }
  void send_tcp_peer_data(std::span<const uint8_t> data) {
    if (!tcp_peer.send_data(data)) {
      log_warn("{} - failed to forward data to TCP peer", ip);
      close();
    }
  }

  void connect_to_tcp_peer(std::string_view address) {
    const auto colon = address.find_last_of(':');
    if (colon == std::string_view::npos) {
      log_warn("{} - invalid TCP peer address {}", ip, address);
      return close();
    }

    const auto hostname = address.substr(0, colon);
    const auto port_string = address.substr(colon + 1);

    uint16_t port{};
    if (!base::text::to_number(port_string, port)) {
      log_warn("{} - invalid TCP peer address {}", ip, address);
      return close();
    }

    log_info("{} - connecting to TCP peer `{}` port {}...", ip, hostname, port);

    tcp_peer = async_net::TcpConnection{*client.io_context(), std::string(hostname), port};
    state = State::Connecting;

    timeout.reset();

    {
      auto self = shared_from_this();
      tcp_peer.set_on_connection_succeeded([self] { self->on_tcp_connection_succeeded(); });
      tcp_peer.set_on_connection_failed(
        [self](async_net::Status status) { self->on_tcp_connection_failed(status); });

      tcp_peer.set_on_disconnected([self] { self->on_tcp_disconnected(); });
      tcp_peer.set_on_error([self](async_net::Status status) { self->on_tcp_error(status); });

      tcp_peer.set_on_data_received(
        [self](std::span<const uint8_t> data) { return self->on_tcp_data_recieved(data); });
    }
  }

  void on_tcp_connection_succeeded() {
    if (state == State::Connecting) {
      log_info("{} - estabilished connection to TCP peer", ip);

      if (client.send_text_message("1")) {
        state = State::Proxying;
      } else {
        log_warn("{} - failed to inform client about successful connection", ip);
        close();
      }
    }
  }

  void on_tcp_connection_failed(async_net::Status status) {
    if (state == State::Connecting) {
      log_warn("{} - failed to connect to TCP peer: {}", ip, status.stringify());

      (void)client.send_text_message("0");
      close();
    }
  }

  void on_tcp_disconnected() {
    log_info("{} - TCP peer disconnected", ip);
    close();
  }

  void on_tcp_error(async_net::Status status) {
    log_warn("{} - TCP peer error {}", ip, status.stringify());
    close();
  }

  size_t on_tcp_data_recieved(std::span<const uint8_t> data) {
    if (!client.send_binary_message(data)) {
      log_warn("{} - failed to forward data to WebSocket client", ip);
      close();
    }

    return data.size();
  }

  void on_binary_message_received(std::span<const uint8_t> data) {
    switch (state) {
      case State::Proxying: {
        send_tcp_peer_data(data);
        break;
      }
      case State::Connecting:
      case State::WaitingForAddress:
      case State::Closed:
      default: {
        log_warn("{} - received unexpected binary message", ip);
        close();
        break;
      }
    }
  }

  void on_text_message_received(std::string_view data) {
    switch (state) {
      case State::WaitingForAddress: {
        connect_to_tcp_peer(data);
        break;
      }
      case State::Proxying: {
        send_tcp_peer_data(data);
        break;
      }
      case State::Connecting:
      case State::Closed:
      default: {
        log_warn("{} - received unexpected text message", ip);
        close();
        break;
      }
    }
  }

 public:
  explicit ProxyClient(async_ws::WebSocketClient client)
      : client(std::move(client)), ip(this->client.peer_address().stringify()) {}

  void startup(std::shared_ptr<ProxyClient> self) {
    client.set_on_disconnected([self] {
      log_info("{} - disconnected", self->ip);
      self->close();
    });
    client.set_on_error([self](async_ws::Status status) {
      log_warn("{} - error {}", self->ip, status.stringify());
      self->close();
    });

    client.set_on_binary_message_received(
      [self](std::span<const uint8_t> data) { return self->on_binary_message_received(data); });
    client.set_on_text_message_received(
      [self](std::string_view data) { return self->on_text_message_received(data); });

    {
      auto weakS = std::weak_ptr<ProxyClient>(self);
      timeout = async_net::Timer::invoke_after(
        *client.io_context(), base::PreciseTime::from_seconds(10), [weakS]() {
          if (auto self = weakS.lock()) {
            if (self->state == State::WaitingForAddress) {
              log_warn("{} - took too long to send TCP peer address", self->ip);
              self->close();
            }
          }
        });
    }
  }
};

int main() {
  base::initialize();

  const uint16_t port = 44444;

  async_net::IoContext context;
  async_ws::WebSocketServer server{context, port};

  server.set_on_listening([port] { log_info("proxy server listening on {}...", port); });
  server.set_on_error([](async_ws::Status status) {
    log_info("failed to start the proxy server: {}", status.stringify());
  });

  server.set_on_client_connected([](std::string_view uri, async_ws::WebSocketClient client) {
    auto proxy_client = std::make_shared<ProxyClient>(std::move(client));
    proxy_client->startup(proxy_client);
  });

  verify(context.run_until_no_work(), "run failed");
}
