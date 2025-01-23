#include "Http.hpp"

#include <base/Panic.hpp>
#include <base/text/Format.hpp>
#include <base/text/Split.hpp>
#include <base/text/Text.hpp>

namespace websocket::http {

static constexpr std::string_view http_line_delimeter = "\r\n";
static constexpr std::string_view http_body_delimeter = "\r\n\r\n";

static std::optional<Method> string_to_method(std::string_view s) {
  // clang-format off
  if (base::text::equals_case_insensitive(s, "GET")) return Method::Get;
  if (base::text::equals_case_insensitive(s, "HEAD")) return Method::Head;
  if (base::text::equals_case_insensitive(s, "OPTIONS")) return Method::Options;
  if (base::text::equals_case_insensitive(s, "TRACE")) return Method::Trace;
  if (base::text::equals_case_insensitive(s, "PUT")) return Method::Put;
  if (base::text::equals_case_insensitive(s, "DELETE")) return Method::Delete;
  if (base::text::equals_case_insensitive(s, "POST")) return Method::Post;
  if (base::text::equals_case_insensitive(s, "PATCH")) return Method::Patch;
  if (base::text::equals_case_insensitive(s, "CONNECT")) return Method::Connect;
  return std::nullopt;
  // clang-format on
}

static std::string_view method_to_string(Method method) {
  // clang-format off
  switch (method) {
    case Method::Get: return "GET";
    case Method::Head: return "HEAD";
    case Method::Options: return "OPTIONS";
    case Method::Trace: return "TRACE";
    case Method::Put: return "PUT";
    case Method::Delete: return "DELETE";
    case Method::Post: return "POST";
    case Method::Patch: return "PATCH";
    case Method::Connect: return "CONNECT";
    default: unreachable();
  }
  // clang-format on
}

std::string_view status_to_string(uint32_t status) {
  // clang-format off
  switch (status) {
    case 100: return "100 Continue";
    case 101: return "101 Switching Protocols";
    case 102: return "102 Processing";
    case 200: return "200 OK";
    case 201: return "201 Created";
    case 202: return "202 Accepted";
    case 203: return "203 Non-Authoritative Information";
    case 204: return "204 No Content";
    case 205: return "205 Reset Content";
    case 206: return "206 Partial Content";
    case 207: return "207 Multi-Status";
    case 208: return "208 Already Reported";
    case 226: return "226 IM Used";
    case 300: return "300 Multiple Choices";
    case 301: return "301 Moved Permanently";
    case 302: return "302 Found";
    case 303: return "303 See Other";
    case 304: return "304 Not Modified";
    case 305: return "305 Use Proxy";
    case 306: return "306 Reserved";
    case 307: return "307 Temporary Redirect";
    case 308: return "308 Permanent Redirect";
    case 400: return "400 Bad Request";
    case 401: return "401 Unauthorized";
    case 402: return "402 Payment Required";
    case 403: return "403 Forbidden";
    case 404: return "404 Not Found";
    case 405: return "405 Method Not Allowed";
    case 406: return "406 Not Acceptable";
    case 407: return "407 Proxy Authentication Required";
    case 408: return "408 Request Timeout";
    case 409: return "409 Conflict";
    case 410: return "410 Gone";
    case 411: return "411 Length Required";
    case 412: return "412 Precondition Failed";
    case 413: return "413 Request Entity Too Large";
    case 414: return "414 Request-URI Too Long";
    case 415: return "415 Unsupported Media Type";
    case 416: return "416 Requested Range Not Satisfiable";
    case 417: return "417 Expectation Failed";
    case 422: return "422 Unprocessable Entity";
    case 423: return "423 Locked";
    case 424: return "424 Failed Dependency";
    case 426: return "426 Upgrade Required";
    case 428: return "428 Precondition Required";
    case 429: return "429 Too Many Requests";
    case 431: return "431 Request Header Fields Too Large";
    case 500: return "500 Internal Server Error";
    case 501: return "501 Not Implemented";
    case 502: return "502 Bad Gateway";
    case 503: return "503 Service Unavailable";
    case 504: return "504 Gateway Timeout";
    case 505: return "505 HTTP Version Not Supported";
    case 506: return "506 Variant Also Negotiates";
    case 507: return "507 Insufficient Storage";
    case 508: return "508 Loop Detected";
    case 510: return "510 Not Extended";
    case 511: return "511 Network Authentication Required";
    default: return "000 Unuspported";
  }
  // clang-format on
}

static std::string serialize_http_request_response(std::string_view first_line,
                                                   const Headers& headers) {
  std::string result{};

  result += first_line;
  result += http_line_delimeter;

  for (const auto& [key, value] : headers.headers) {
    result += base::format("{}: {}{}", key, value, http_line_delimeter);
  }

  result += headers.headers.empty() ? http_body_delimeter : http_line_delimeter;

  return result;
}

void Headers::set(std::string key, std::string value) {
  for (auto& [header_key, header_value] : headers) {
    if (base::text::equals_case_insensitive(header_key, key)) {
      header_value = std::move(value);
      return;
    }
  }

  headers.emplace_back(std::move(key), std::move(value));
}

std::optional<std::string_view> Headers::get(std::string_view key) const {
  for (const auto& [header_key, header_value] : headers) {
    if (base::text::equals_case_insensitive(header_key, key)) {
      return std::string_view{header_value};
    }
  }

  return std::nullopt;
}

static std::string serialize_version(const Version& version) {
  if ((version.major == 0 || version.major == 1) || version.minor != 0) {
    return base::format("HTTP/{}.{}", version.major, version.minor);
  } else {
    return base::format("HTTP/{}", version.major);
  }
}

std::string Serializer::serialize(const Request& request) {
  const auto first_line = base::format("{} {} {}", method_to_string(request.method), request.uri,
                                       serialize_version(request.version));
  return serialize_http_request_response(first_line, request.headers);
}

std::string Serializer::serialize(const Response& response) {
  const auto first_line =
    base::format("{} {}", serialize_version(response.version), status_to_string(response.status));
  return serialize_http_request_response(first_line, response.headers);
}

template <typename T, typename ParseFirstLine, typename Finalize>
Deserializer::Result<T> deserialize_request_response(std::string_view data,
                                                     ParseFirstLine&& parse_first_line,
                                                     Finalize&& finalize) {
  constexpr size_t max_http_first_line_size = 512;
  constexpr size_t max_http_request_response_size = 8192;

  const auto first_line_end_index = data.find(http_line_delimeter);
  if (first_line_end_index == std::string_view::npos) {
    if (data.size() > max_http_first_line_size) {
      return {.error = Deserializer::Error::TooLarge};
    } else {
      return {.error = Deserializer::Error::NotEnoughData};
    }
  }

  const auto first_line = data.substr(0, first_line_end_index);
  auto [first_line_error, first_line_parsed] = parse_first_line(first_line);
  if (first_line_error != Deserializer::Error::Ok) {
    return {.error = first_line_error};
  }

  const auto request_response_end_index = data.find(http_body_delimeter);
  if (request_response_end_index == std::string_view::npos) {
    if (data.size() > max_http_request_response_size) {
      return {.error = Deserializer::Error::TooLarge};
    } else {
      return {.error = Deserializer::Error::NotEnoughData};
    }
  }

  const auto consumed_size = request_response_end_index + http_body_delimeter.size();
  if (consumed_size > max_http_request_response_size) {
    return {.error = Deserializer::Error::TooLarge};
  }

  const auto whole_request_response = data.substr(0, request_response_end_index);
  const auto unparsed_headers =
    whole_request_response.substr(first_line_end_index + http_line_delimeter.size());

  Headers headers;

  const auto split_result = base::text::split(
    unparsed_headers, http_line_delimeter, base::text::TrailingDelimeter::Ignore,
    [&](std::string_view part) {
      if (part.empty()) {
        return true;
      }

      std::array<std::string_view, 2> key_value{};
      if (!base::text::splitn_to(part, ":", base::text::TrailingDelimeter::Ignore, key_value)) {
        return false;
      }

      const auto key = base::text::strip(key_value[0]);
      const auto value = base::text::strip(key_value[1]);
      if (key.empty()) {
        return true;
      }

      headers.set(std::string(key), std::string(value));

      return true;
    });

  if (!split_result) {
    return {.error = Deserializer::Error::InvalidHeaders};
  }

  return {
    .error = Deserializer::Error::Ok,
    .consumed_size = consumed_size,
    .deserialized = finalize(first_line_parsed, std::move(headers)),
  };
}

static std::optional<Version> deserialize_version(std::string_view version) {
  constexpr std::string_view version_start = "HTTP/";
  if (version.size() <= version_start.size() ||
      !base::text::equals_case_insensitive(version.substr(0, version_start.size()),
                                           version_start)) {
    return std::nullopt;
  }

  const auto version_number_as_string = version.substr(version_start.size());

  std::array<std::string_view, 2> version_parts{};
  if (!base::text::split_to(version_number_as_string, ".", base::text::TrailingDelimeter::Handle,
                            version_parts)) {
    Version result_version{};
    if (!base::text::to_number<uint32_t>(version_number_as_string, result_version.major)) {
      return std::nullopt;
    }

    return result_version;
  }

  Version result_version{};
  if (!base::text::to_number<uint32_t>(version_parts[0], result_version.major) ||
      !base::text::to_number<uint32_t>(version_parts[1], result_version.minor)) {
    return std::nullopt;
  }

  return result_version;
}

Deserializer::Result<Request> Deserializer::deserialize_request(std::string_view data) {
  struct Context {
    Method method{};
    std::string uri{};
    Version version{};
  };
  return deserialize_request_response<Request>(
    data,
    [&](std::string_view first_line) -> std::pair<Error, Context> {
      std::array<std::string_view, 3> parts{};
      if (!base::text::splitn_to(first_line, " ", base::text::TrailingDelimeter::Ignore, parts)) {
        return {Error::MalformedFirstLine, {}};
      }

      const auto method = string_to_method(parts[0]);
      if (!method) {
        return {Error::InvalidMethod, {}};
      }

      const auto version = deserialize_version(parts[2]);
      if (!version) {
        return {Error::InvalidVersion, {}};
      }

      return {Error::Ok, Context{
                           .method = *method,
                           .uri = std::string(parts[1]),
                           .version = *version,
                         }};
    },
    [&](Context& context, Headers headers) -> Request {
      return Request{
        .method = context.method,
        .uri = std::move(context.uri),
        .version = context.version,
        .headers = std::move(headers),
      };
    });
}

Deserializer::Result<Response> Deserializer::deserialize_response(std::string_view data) {
  struct Context {
    Version version{};
    uint32_t status{};
  };
  return deserialize_request_response<Response>(
    data,
    [&](std::string_view first_line) -> std::pair<Error, Context> {
      std::array<std::string_view, 2> parts{};
      if (!base::text::splitn_to(first_line, " ", base::text::TrailingDelimeter::Ignore, parts)) {
        return {Error::MalformedFirstLine, {}};
      }

      const auto version = deserialize_version(parts[0]);
      if (!version) {
        return {Error::InvalidVersion, {}};
      }

      std::array<std::string_view, 2> status_parts{};
      if (!base::text::splitn_to(parts[1], " ", base::text::TrailingDelimeter::Ignore,
                                 status_parts)) {
        return {Error::InvalidStatus, {}};
      }

      uint32_t status{};
      if (!base::text::to_number<uint32_t>(status_parts[0], status)) {
        return {Error::InvalidStatus, {}};
      }

      return {Error::Ok, Context{
                           .version = *version,
                           .status = status,
                         }};
    },
    [&](Context& context, Headers headers) -> Response {
      return Response{
        .version = context.version,
        .status = context.status,
        .headers = std::move(headers),
      };
    });
}

Deserializer::Result<Request> Deserializer::deserialize_request(std::span<const uint8_t> data) {
  return deserialize_request(
    std::string_view{reinterpret_cast<const char*>(data.data()), data.size()});
}

Deserializer::Result<Response> Deserializer::deserialize_response(std::span<const uint8_t> data) {
  return deserialize_response(
    std::string_view{reinterpret_cast<const char*>(data.data()), data.size()});
}

}  // namespace websocket::http