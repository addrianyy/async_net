#include "ChatServer.hpp"

#include <async_net/IoContext.hpp>

#include <base/Initialization.hpp>
#include <base/Panic.hpp>

int main() {
  base::initialize();

  const uint16_t port = 44444;

  async_net::IoContext context;
  ChatServer server{context, port};

  verify(context.run_until_no_work(), "run failed");
}