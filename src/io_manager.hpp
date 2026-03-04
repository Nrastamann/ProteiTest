#include <iostream>
#include <memory>
#include <ostream>
#pragma once

class IOManager {
  std::shared_ptr<std::istream> _in;
  std::shared_ptr<std::ostream> _out;

 public:
  IOManager()
  {
    _in.reset(&std::cin, []<typename... Args>(Args...) {});
    _out.reset(&std::cout, []<typename... Args>(Args...) {});
  }
  void setTestEnvironmentStatic(std::istream* ptr_in, std::ostream* ptr_out)
  {
    _in.reset(ptr_in, []<typename... Args>(Args...) {});
    _out.reset(ptr_out, []<typename... Args>(Args...) {});
  }

  void setTestEnvironment(std::istream* ptr_in, std::ostream* ptr_out)
  {
    _in.reset(ptr_in);
    _out.reset(ptr_out);
  }
  ~IOManager() = default;
  std::ostream& cout() { return *_out.get(); }
  std::istream& cin() { return *_in.get(); }
  IOManager(IOManager&&) = default;
  IOManager(IOManager&) = delete;
  IOManager& operator=(IOManager&&) = default;
  IOManager& operator=(IOManager&) = delete;
};

namespace protei_io {
inline IOManager& io()
{
  static IOManager io_manager;
  return io_manager;
}
};  // namespace protei_io
