#include <base/Initialization.hpp>
#include <base/Log.hpp>
#include <base/Panic.hpp>

#include <async_net/IoContext.hpp>
#include <async_net/TcpConnection.hpp>

int main() {
  base::initialize();

  const uint16_t port = 8000;

  async_net::IoContext context;
  async_net::TcpConnection connection{context, "localhost", port};

  connection.set_on_connected([&](async_net::Status status) {
    log_info("connection status: {}", status.stringify());
    connection.shutdown();
  });

  verify(context.run_until_no_work(), "run failed");
}
