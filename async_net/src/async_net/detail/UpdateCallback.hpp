#pragma once
#include <async_net/IoContext.hpp>

namespace async_net::detail {

template <typename T>
void update_callback(IoContext& context, T& callback_member, T new_callback) {
  if (callback_member) {
    context.post_destroy(std::move(callback_member));
  }
  callback_member = std::move(new_callback);
}

}  // namespace async_net::detail
