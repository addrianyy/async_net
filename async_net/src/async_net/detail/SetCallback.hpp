#pragma once

#define SET_CALLBACK_SAFE(member)                                                              \
  if (impl_) {                                                                                 \
    if (impl_->member && !instant) {                                                           \
      impl_->context.post(                                                                     \
        [self = impl_, cb = std::move(callback)]() mutable { self->member = std::move(cb); }); \
    } else {                                                                                   \
      impl_->member = std::move(callback);                                                     \
    }                                                                                          \
  }
