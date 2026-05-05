#pragma once

#if defined(_WIN32)
#define SOCKLIB_WINDOWS
#elif defined(__linux__)
#define SOCKLIB_LINUX
#elif defined(__APPLE__)
#define SOCKLIB_APPLE
#else
#error "Unsupported platform"
#endif

#if defined(SOCKLIB_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <afunix.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
