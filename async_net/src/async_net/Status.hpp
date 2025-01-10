#pragma once
#include <socklib/Status.hpp>

namespace async_net {

using Error = sock::Error;
using SystemError = sock::SystemError;
using Status = sock::Status;

}  // namespace async_net