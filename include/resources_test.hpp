#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <string_view>
#include "ip_addr.hpp"
#include "logger.hpp"

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
    if (_resources.size() != 0 &&
        !std::ranges::all_of(
            _resources.begin(), _resources.end(),
            [](std::string_view sv) { return std::filesystem::exists(sv); })) {

      logger_presets::acquiringResourceError<ResourceTest>(
          std::format("{}", _resources));
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
  explicit ConnectionTest(std::span<IpAddr> addresses) : _resources(addresses)
  {
  }

  bool operator()() override
  {
    bool value = true;

    auto test_resource = [](const IpAddr& addr) {
      int client_socket = socket(AF_INET, SOCK_STREAM, 0);

      const sockaddr_in server_addr{.sin_family = AF_INET,
                                    .sin_port = htons(addr._port),
                                    .sin_addr{addr.htonlP()},
                                    .sin_zero{0}};

      int res = connect(client_socket,
                        reinterpret_cast<const sockaddr*>(&server_addr),
                        sizeof(server_addr));
      if (res != 0) {
        return false;
      }
      close(client_socket);
      return true;
    };

    if (_invalid_state || (_resources.size() != 0 &&
                           !std::ranges::all_of(_resources, test_resource))) {
      std::string output_str;
      for (auto& ip_addr : _resources) {
        output_str += std::format("{}\n", ip_addr);
      }
      logger_presets::acquiringResourceError<ConnectionTest>(output_str);

      value = false;
    }

    return value;
  };

 private:
  std::span<IpAddr> _resources;

  bool _invalid_state = false;
};
