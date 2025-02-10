#include <base/Initialization.hpp>
#include <base/Log.hpp>
#include <base/Panic.hpp>

#include <async_net/IoContext.hpp>
#include <async_net/UdpSocket.hpp>

int main() {
  base::initialize();

  const uint16_t port = 44444;

  async_net::IoContext context;
  async_net::UdpSocket socket{context, port};

  socket.set_on_bound([](async_net::Status status) {
    if (status) {
      log_info("binding succeeded");
    } else {
      log_info("binding failed: {}", status.stringify());
    }
  });

  socket.set_on_data_received(
    [&socket](const async_net::SocketAddress& peer, std::span<const uint8_t> data) {
      log_info("data received ({} bytes) from {}", data.size(), peer.stringify());
      socket.send_data(peer, data);
    });

  verify(context.run_until_no_work(), "run failed");
}
