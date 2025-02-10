#include "ChatClient.hpp"
#include "ChatServer.hpp"

#include <base/Log.hpp>

static bool is_valid_username(std::string_view username) {
  if (username.empty()) {
    return false;
  }

  for (const char c : username) {
    if (!std::isalnum(c) && c != '_') {
      return false;
    }
  }

  return true;
}

static bool is_valid_chat_message(std::string_view message) {
  for (const char c : message) {
    if (c == '\r' || c == '\n') {
      return false;
    }
  }

  return true;
}

void ChatClient::close() {
  if (state != State::Closed) {
    if (state == State::Chatting) {
      server.unregister_client(this);
    }
    client.shutdown();
    state = State::Closed;
  }
}

void ChatClient::process_message(std::string_view message) {
  switch (state) {
    case State::WaitingForUsername: {
      const auto requested_username = std::string(message);

      if (is_valid_username(requested_username)) {
        log_info("{} - username `{}`", ip, requested_username);

        username = std::string(requested_username);
        state = State::Chatting;

        server.register_client(shared_from_this());
      } else {
        log_warn("{} - rejected username", ip);
        send("Username rejected");
        close();
      }

      break;
    }

    case State::Chatting: {
      if (is_valid_chat_message(message)) {
        log_info("{} ({}) - message `{}`", ip, username, message);
        server.on_message(this, message);
      } else {
        log_warn("{} ({}) - rejected message", ip, username);
      }
      break;
    }

    case State::Closed:
    default: {
      break;
    }
  }
}

ChatClient::ChatClient(ChatServer& server, async_ws::WebSocketClient client)
    : server(server), client(std::move(client)), ip(this->client.peer_address().stringify()) {}

void ChatClient::startup(std::shared_ptr<ChatClient> self) {
  log_info("{} - connected", ip);

  client.set_on_text_message_received(
    [self](std::string_view message) { self->process_message(message); });

  client.set_on_closed([self](async_ws::Status status) {
    log_info("{} - connection closed (error {})", self->ip, status.stringify());
    self->close();
  });
}

void ChatClient::send(std::string_view message) {
  (void)client.send_text_message(message);
}
