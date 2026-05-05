#pragma once
#include "StreamSocket.hpp"

#include <utility>

namespace sock {

class ConnectedPair {
 public:
  struct ConnectedPairParameters {
    bool non_blocking = false;

    constexpr static ConnectedPairParameters default_parameters() {
      return ConnectedPairParameters{};
    }
  };

  static Result<std::pair<StreamSocket, StreamSocket>> create(
    const ConnectedPairParameters& pair_parameters = ConnectedPairParameters::default_parameters()
  );
};

}  // namespace sock
