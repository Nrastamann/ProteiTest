#pragma once

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <cassert>
#include <filesystem>
#include <format>
#include <string_view>
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
  ConnectionTest(std::span<std::array<uint8_t, 4>> resources,
                 std::span<size_t> ports)
      : _resources(resources.size())
  {

    if (ports.size() != resources.size()) {
      _invalid_state = true;
      return;
    }
    std::ranges::transform(
        resources, ports, _resources.begin(),
        [](const std::array<uint8_t, 4>& ip_addr, size_t port) {
          return std::pair(ip_addr, port);
        });
  }

  bool operator()() override
  {
    bool value = true;

    auto test_resource =
        [](const std::pair<std::array<uint8_t, 4>, size_t>& addr) {
          using namespace boost::asio;

          boost::asio::io_context service;
          ip::tcp::endpoint ep(ip::make_address_v4(addr.first),
                               static_cast<unsigned short>(addr.second));
          ip::tcp::socket sock(service);
          boost::system::error_code err;
          err = sock.connect(ep, err);

          if (err) {
            return false;
          }

          sock.cancel();
          return true;
        };

    if (_invalid_state || (_resources.size() != 0 &&
                           !std::ranges::all_of(_resources, test_resource))) {
      logger_presets::acquiringResourceError<ConnectionTest>(
          std::format("{} - addresses/ports, {} - invalid size", _resources,
                      _invalid_state));
      value = false;
    }

    return value;
  };

 private:
  std::vector<std::pair<std::array<uint8_t, 4>, size_t>> _resources;

  bool _invalid_state = false;
};
