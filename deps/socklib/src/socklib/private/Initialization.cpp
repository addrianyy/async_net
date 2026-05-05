#include "Initialization.hpp"

namespace sock::detail {

#if defined(SOCKLIB_WINDOWS)
bool initialize_socket_library() {
  class InitializationGuard {
    bool status_ = false;

   public:
    InitializationGuard() {
      WSAData wsa_data{};
      const auto version = MAKEWORD(2, 2);
      status_ = WSAStartup(version, &wsa_data) == 0;
      if (wsa_data.wVersion != version) {
        status_ = false;
      }
    }

    bool status() const { return status_; }
  };
  static InitializationGuard guard;
  return guard.status();
}
#endif

}  // namespace sock::detail
