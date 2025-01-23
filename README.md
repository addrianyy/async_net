## Asynchronous, multi-platform networking library for TCP, UDP and WebSocket

- async_net: async networking library for TCP and UDP
- async_ws: async WebSocket implementation based on async_net

```c++
const uint16_t port = 44444;

async_net::IoContext context;
async_ws::WebSocketServer server{context, port};

server.set_on_listening([port] { log_info("server listening on {}...", port); });
server.set_on_error(
  [](async_ws::Status status) { log_info("failed to start the server: {}", status.stringify()); });

server.set_on_client_connected([](std::string_view uri, async_ws::WebSocketClient client) {
  (void)client.send_text_message("Hello world!");
});

verify(context.run_until_no_work(), "run failed");
```