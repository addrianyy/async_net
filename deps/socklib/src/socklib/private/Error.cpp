#include "Error.hpp"

namespace sock::detail {

Status system_error_to_status(int error_num) {
  using EC = Status;

#if defined(SOCKLIB_WINDOWS)
  switch (error_num) {
    case WSAEISCONN:        return EC::AlreadyConnected;
    case WSAENOTCONN:       return EC::NotConnected;
    case WSANOTINITIALISED: return EC::NotInitialized;
    case WSAENETDOWN:       return EC::NetworkSubsystemFailed;
    case WSAEACCES:         return EC::AccessDenied;
    case WSAEADDRINUSE:     return EC::AddressInUse;
    case WSAECONNREFUSED:   return EC::ConnectionRefused;
    case ENETUNREACH:       return EC::NetworkUnreachable;
    case WSAETIMEDOUT:      return EC::TimedOut;
    case WSAEWOULDBLOCK:    return EC::WouldBlock;
    case WSAEALREADY:       return EC::AlreadyInProgress;
    case WSAEINPROGRESS:    return EC::NowInProgress;
    case WSAEHOSTUNREACH:   return EC::HostUnreachable;
    case WSAEBADF:
    case WSAENOTSOCK:       return EC::InvalidSocket;
    case WSAECONNRESET:     return EC::ConnectionReset;
    case WSAEDESTADDRREQ:   return EC::NoPeerAddress;
    case WSAESHUTDOWN:      return EC::SocketShutdown;
    case WSAEADDRNOTAVAIL:  return EC::AddressNotAvailable;
    case WSAEINVAL:         return EC::InvalidValue;
    default:                return EC::Unknown;
  }
#else
  switch (error_num) {
    case EISCONN:       return EC::AlreadyConnected;
    case ENOTCONN:      return EC::NotConnected;
    case ENETDOWN:      return EC::NetworkSubsystemFailed;
    case EACCES:
    case EPERM:         return EC::AccessDenied;
    case EADDRINUSE:    return EC::AddressInUse;
    case ECONNREFUSED:  return EC::ConnectionRefused;
    case ENETUNREACH:   return EC::NetworkUnreachable;
    case ETIMEDOUT:     return EC::TimedOut;
    case EWOULDBLOCK:   return EC::WouldBlock;
    case EALREADY:      return EC::AlreadyInProgress;
    case EINPROGRESS:   return EC::NowInProgress;
    case EHOSTUNREACH:  return EC::HostUnreachable;
    case EBADF:
    case ENOTSOCK:      return EC::InvalidSocket;
    case ECONNRESET:    return EC::ConnectionReset;
    case EDESTADDRREQ:  return EC::NoPeerAddress;
    case EPIPE:         return EC::SocketShutdown;
    case EADDRNOTAVAIL: return EC::AddressNotAvailable;
    case EINVAL:        return EC::InvalidValue;
    default:            return EC::Unknown;
  }
#endif
}

Status system_last_error_to_status() {
#if defined(SOCKLIB_WINDOWS)
  return system_error_to_status(WSAGetLastError());
#else
  return system_error_to_status(errno);
#endif
}

}  // namespace sock::detail
