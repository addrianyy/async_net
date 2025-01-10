#pragma once

#define SET_CALLBACK_SAFE(member)                                                    \
  if (member && !instant) {                                                          \
    auto selfCopy = self;                                                            \
    context.post([selfS = std::move(selfCopy), cb = std::move(callback)]() mutable { \
      selfS->member = std::move(cb);                                                 \
    });                                                                              \
  } else {                                                                           \
    member = std::move(callback);                                                    \
  }
