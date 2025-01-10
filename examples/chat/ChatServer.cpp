#include "ChatServer.hpp"
#include "ChatClient.hpp"

#include <base/Log.hpp>
#include <base/text/Format.hpp>

void ChatServer::broadcast(std::string_view message) {
  for (auto& client : clients) {
    client->send(message);
  }
}

ChatServer::ChatServer(async_net::IoContext& context, uint32_t port) : server(context, port) {
  server.set_on_listening([port] { log_info("chat server listening on {}...", port); });
  server.set_on_error([](async_ws::Status status) {
    log_info("failed to start the chat server: {}", status.stringify());
  });

  server.set_on_client_connected([this](std::string_view uri, async_ws::WebSocketClient client) {
    auto chat_client = std::make_shared<ChatClient>(*this, std::move(client));
    chat_client->startup(chat_client);
  });
}

void ChatServer::register_client(std::shared_ptr<ChatClient> client) {
  const auto info = base::format("{} connected", client->name());

  clients.push_back(std::move(client));

  broadcast(info);
}

void ChatServer::unregister_client(const ChatClient* client) {
  const auto info = base::format("{} disconnected", client->name());

  const auto removed_count =
    std::erase_if(clients, [&](const std::shared_ptr<ChatClient>& connected_client) {
      return connected_client.get() == client;
    });

  if (removed_count > 0) {
    broadcast(info);
  }
}

void ChatServer::on_message(const ChatClient* from, std::string_view message) {
  const auto info = base::format("{}: {}", from->name(), message);
  broadcast(info);
}