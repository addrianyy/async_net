#include "ConnectedPair.hpp"
#include "private/Error.hpp"
#include "private/Initialization.hpp"
#include "private/System.hpp"

#if defined(SOCKLIB_WINDOWS)
#define SOCKLIB_EMULATED_SOCKET_PAIR
#endif

#if defined(SOCKLIB_EMULATED_SOCKET_PAIR)
#include "ConnectingStreamSocket.hpp"
#include "Listener.hpp"
#endif

namespace sock {

#if defined(SOCKLIB_EMULATED_SOCKET_PAIR)
template <typename Address>
static Result<std::pair<StreamSocket, StreamSocket>> socket_pair_emulated(bool non_blocking) {
  auto listener = Listener::bind(
    Address{Address::Ip::loopback(), 0},
    {
      .non_blocking = true,
      .max_pending_connections = 1,
    }
  );
  if (!listener) {
    return std::unexpected{listener.error()};
  }

  const auto local_address = listener->template local_address<Address>();
  if (!local_address) {
    return std::unexpected{local_address.error()};
  }

  auto connection =
    ConnectingStreamSocket::initiate(Address{Address::Ip::loopback(), local_address->port()});
  if (!connection) {
    return std::unexpected{connection.error()};
  }

  auto accepted = listener->accept();
  if (!accepted) {
    return std::unexpected{accepted.error()};
  }

  StreamSocket socket_1 = std::move(*accepted);
  StreamSocket socket_2 = std::move(connection->connected);
  if (!socket_2) {
    auto connection_2 = connection->connecting.connect();
    if (!connection_2) {
      return std::unexpected{connection_2.error()};
    }
    socket_2 = std::move(*connection_2);
  }

  if (auto status = socket_1.set_non_blocking(non_blocking); status != Status::Ok) {
    return std::unexpected{status};
  }
  if (auto status = socket_2.set_non_blocking(non_blocking); status != Status::Ok) {
    return std::unexpected{status};
  }

  return std::pair{std::move(socket_1), std::move(socket_2)};
}
#endif

Result<std::pair<StreamSocket, StreamSocket>> ConnectedPair::create(
  const ConnectedPairParameters& pair_parameters
) {
  SOCKLIB_ENSURE_INITIALIZED();

#if defined(SOCKLIB_EMULATED_SOCKET_PAIR)
  Status last_status = Status::Ok;

  for (int i = 0; i < 4; ++i) {
    {
      auto pair = socket_pair_emulated<SocketIpV6Address>(pair_parameters.non_blocking);
      if (pair) {
        return pair;
      }
      last_status = pair.error();
    }
    {
      auto pair = socket_pair_emulated<SocketIpV4Address>(pair_parameters.non_blocking);
      if (pair) {
        return pair;
      }
      last_status = pair.error();
    }
  }

  return std::unexpected{last_status};
#else
  detail::RawSocket sockets[2]{};
  if (detail::is_error(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))) {
    return std::unexpected{detail::system_last_error_to_status()};
  }
  return std::pair{StreamSocket{sockets[0]}, StreamSocket{sockets[1]}};
#endif
}

}  // namespace sock
