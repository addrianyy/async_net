#pragma once
#include <async_ws/WebSocketClient.hpp>

#include <string>
#include <string_view>

class ChatServer;

class ChatClient : public std::enable_shared_from_this<ChatClient> {
  ChatServer& server;
  async_ws::WebSocketClient client;

  std::string ip;
  std::string username;

  enum class State {
    WaitingForUsername,
    Chatting,
    Closed,
  };
  State state{State::WaitingForUsername};

  void close();
  void process_message(std::string_view message);

 public:
  ChatClient(ChatServer& server, async_ws::WebSocketClient client);

  void startup(std::shared_ptr<ChatClient> self);
  void send(std::string_view message);

  std::string_view name() const { return username; }
};