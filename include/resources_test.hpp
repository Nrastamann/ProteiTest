#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <iterator>
#include <string_view>
#include "ip_addr.hpp"
#include "logger.hpp"

template <>
struct std::formatter<std::span<std::string_view>>
    : std::formatter<std::string> {
  auto format(std::span<std::string_view> vec, std::format_context& ctx) const
  {
    std::string out = "[ ";
    for (const auto& i : vec) {
      out += i;
    }
    out += " ]";
    return std::formatter<std::string>::format(out, ctx);
  }
};

namespace resources_tests {
class ITest {
 public:
  virtual bool operator()() = 0;

  ITest() = default;
  ITest(const ITest&) = default;
  ITest(ITest&&) = default;
  ITest& operator=(const ITest&) = default;
  ITest& operator=(ITest&&) = default;
  virtual ~ITest() = default;
};

class ResourceTest final : ITest {
 public:
  explicit ResourceTest(std::span<std::string_view> resources)
      : _resources(resources)
  {
  }

  ~ResourceTest() final = default;
  ResourceTest(const ResourceTest&) = default;
  ResourceTest(ResourceTest&&) = default;
  ResourceTest& operator=(const ResourceTest&) = default;
  ResourceTest& operator=(ResourceTest&&) = default;

  bool operator()() override
  {
    bool value = true;
    auto bad_it{_resources.begin()};

    if (_resources.size() != 0 &&
        !std::ranges::all_of(_resources.begin(), _resources.end(),
                             [&bad_it](std::string_view sv) {
                               std::advance(bad_it, 1);
                               return std::filesystem::exists(sv);
                             })) {

      std::string output_str = std::format("{}", *(bad_it - 1));
      logging::logger_presets::acquiringResourceError<ResourceTest>(output_str);

      value = false;
    }

    return value;
  };

 private:
  std::span<std::string_view> _resources;
};

class ConnectionTest final : ITest {
 public:
  ~ConnectionTest() final = default;
  ConnectionTest(const ConnectionTest&) = default;
  ConnectionTest(ConnectionTest&&) = default;
  ConnectionTest& operator=(const ConnectionTest&) = default;
  ConnectionTest& operator=(ConnectionTest&&) = default;
  ConnectionTest(std::span<network_addr::IpAddr> addresses)
      : _resources(addresses)
  {
  }

  bool operator()() override
  {
    bool value = true;
    auto bad_it{_resources.begin()};

    auto test_resource = [&bad_it](const network_addr::IpAddr& addr) {
      int client_socket = socket(AF_INET, SOCK_STREAM, 0);
      if (client_socket == -1) {
        return false;
      }

      sockaddr_in server_addr{.sin_family = AF_INET,
                              .sin_port = htons(addr._port),
                              .sin_addr{addr.addrToNetwork()},
                              .sin_zero{0}};

      int res =
          connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr),
                  sizeof(server_addr));
      if (res != 0) {
        return false;
      }

      std::advance(bad_it, 1);

      close(client_socket);
      return true;
    };

    if (_invalid_state || (_resources.size() != 0 &&
                           !std::ranges::all_of(_resources, test_resource))) {

      std::string output_str = std::format("{}", *bad_it);

      logging::logger_presets::acquiringResourceError<ConnectionTest>(
          output_str);

      value = false;
    }

    return value;
  };

 private:
  std::span<network_addr::IpAddr> _resources;

  bool _invalid_state = false;
};
}  // namespace resources_tests
