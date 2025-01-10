#pragma once
#include <async_ws/WebSocketServer.hpp>

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

class ChatClient;

class ChatServer {
  async_ws::WebSocketServer server;
  std::vector<std::shared_ptr<ChatClient>> clients;

  void broadcast(std::string_view message);

 public:
  ChatServer(async_net::IoContext& context, uint32_t port);

  void register_client(std::shared_ptr<ChatClient> client);
  void unregister_client(const ChatClient* client);

  void on_message(const ChatClient* from, std::string_view message);
};