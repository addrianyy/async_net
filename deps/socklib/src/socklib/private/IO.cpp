#include "IO.hpp"
#include "SocketHelpers.hpp"

namespace sock::detail {

Result<size_t> socket_receive_sg(
  RawSocket socket,
  std::span<const ScatterGather::Entry> sg,
  void* source_address,
  size_t source_address_size
) {
  msghdr msghdr{
    .msg_name = source_address,
    .msg_namelen = static_cast<socklen_t>(source_address_size),
    .msg_iov = const_cast<iovec*>(sg.data()),
    .msg_iovlen = static_cast<decltype(msghdr.msg_iovlen)>(sg.size()),
  };

  const auto result = retry_on_eintr([&] { return recvmsg(socket, &msghdr, 0); });
  if (result > 0) {
    return result;
  }
  if (result == 0) {
    return std::unexpected{Status::Disconnected};
  }
  return std::unexpected{system_last_error_to_status()};
}

Result<size_t> socket_send_sg(
  RawSocket socket,
  std::span<const ScatterGather::Entry> sg,
  const void* dest_address,
  size_t dest_address_size
) {
  msghdr msghdr{
    .msg_name = const_cast<void*>(dest_address),
    .msg_namelen = static_cast<socklen_t>(dest_address_size),
    .msg_iov = const_cast<iovec*>(sg.data()),
    .msg_iovlen = static_cast<decltype(msghdr.msg_iovlen)>(sg.size()),
  };

  const auto result = retry_on_eintr([&] { return sendmsg(socket, &msghdr, MSG_NOSIGNAL); });
  if (result > 0) {
    return result;
  }
  if (result == 0) {
    return std::unexpected{Status::Disconnected};
  }
  return std::unexpected{system_last_error_to_status()};
}

Result<size_t> socket_receive(
  RawSocket socket, std::span<uint8_t> data, void* source_address, size_t source_address_size
) {
  ScatterGather::Entry sge;
  ScatterGather::set_span(sge, data);
  return socket_receive_sg(socket, std::span{&sge, 1}, source_address, source_address_size);
}

Result<size_t> socket_send(
  RawSocket socket,
  std::span<const uint8_t> data,
  const void* dest_address,
  size_t dest_address_size
) {
  ScatterGather::Entry sge;
  ScatterGather::set_span(sge, data);
  return socket_send_sg(socket, std::span{&sge, 1}, dest_address, dest_address_size);
}

}  // namespace sock::detail
