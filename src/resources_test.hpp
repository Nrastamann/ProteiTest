#pragma once

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <filesystem>
#include <iostream>
#include <string_view>

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
    assert(std::filesystem::exists(_resources[0]));

    return _resources.size() == 0
               ? true
               : std::ranges::all_of(_resources.begin(), _resources.end(),
                                     [](std::string_view sv) {
                                       return std::filesystem::exists(sv);
                                     });
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
  ConnectionTest(std::span<std::array<uint8_t, 4>> resources, size_t port)
      : _resources(resources), _port(port)
  {
  }
  bool operator()() override
  {
    return _resources.size() == 0
               ? true
               : std::ranges::all_of(
                     _resources,
                     [&port = _port](const std::array<uint8_t, 4>& addr) {
                       using namespace boost::asio;

                       std::cout << std::endl;
                       boost::asio::io_context service;
                       ip::tcp::endpoint ep(ip::make_address_v4(addr),
                                            static_cast<unsigned short>(port));
                       ip::tcp::socket sock(service);
                       boost::system::error_code err;
                       err = sock.connect(ep, err);

                       if (err) {
                         return false;
                       }

                       sock.close();
                       return true;
                     });
  };

 private:
  std::span<std::array<uint8_t, 4>> _resources;
  size_t _port;
};
