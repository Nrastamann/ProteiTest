#include <benchmark/benchmark.h>
#include <unistd.h>
#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "custom_types.hpp"
#include "data_pool.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"

static constexpr size_t kTestCount{100};

class InputFixture : public benchmark::Fixture {
  static constexpr size_t kIndexTest{42};

 protected:
  void SetUp(::benchmark::State& /*state*/) override
  {
    _cinBuf = std::cin.rdbuf();
    _coutBuf = std::cout.rdbuf();
    std::cin.rdbuf(_cin.rdbuf());
    std::cout.rdbuf(_cout.rdbuf());
  }

  void TearDown(::benchmark::State& /*state*/) override
  {
    std::cin.rdbuf(_cinBuf);
    std::cout.rdbuf(_coutBuf);
  }

  std::stringstream _cin;
  std::stringstream _cout;
  std::streambuf* _cinBuf{};
  std::streambuf* _coutBuf{};

  Menu _menu;
  AppSettings _command_line_options{{}, {}, {}, "", kIndexTest};
  data_storage::DataPool _data_pool;
  FunctionArgs _arguments{_command_line_options, _data_pool};
};

class ArgvFixture : public benchmark::Fixture {
 protected:
  std::vector<const char*> _argv = {"./build/Debug/proteip",
                                    "-a",
                                    "ff",
                                    "00",
                                    "ff",
                                    "ff:3000",
                                    "-a",
                                    "127",
                                    "0",
                                    "0",
                                    "1",
                                    "-a",
                                    "122.12.122.122:5000",
                                    "-a",
                                    "127",
                                    "127",
                                    "127",
                                    "127:2228",
                                    "-a",
                                    "127",
                                    "127",
                                    "127",
                                    "127",
                                    "2229",
                                    "-a",
                                    "127.127.127",
                                    "127:2230",
                                    "-a",
                                    "127",
                                    ".127",
                                    "127",
                                    ".127:2231",
                                    "-i",
                                    "12"};

  std::vector<std::string_view> _parsed_data = {"-a", "ff.00.ff.ff", "-p", "3000",
                                                "-a", "127.0.0.1",   "-a", "122.12.122.122",
                                                "-p", "5000",        "-a", "127.127.127.127",
                                                "-p", "2228",        "-a", "127.127.127.127",
                                                "-p", "2229",        "-a", "127.127.127.127",
                                                "-p", "2230",        "-a", "127.127.127.127",
                                                "-p", "2231",        "-i", "12"};
};

class ParsingFixture : public benchmark::Fixture {
 protected:
  static constexpr std::array<uint8_t, 4> kTestIPAddr{127, 0, 0, 1};
  static constexpr size_t kTestPort1{3000};
  static constexpr size_t kTestPort2{3001};
  static constexpr size_t kTestIndex{42};
  static constexpr std::string_view kTestRole{"User"};
  static constexpr std::string_view kTestLib{"src"};

  std::vector<std::string> _parsed_data = {
      "-a",
      std::format("{}.0{}.0{}.0{}", kTestIPAddr.at(0), kTestIPAddr.at(1), kTestIPAddr.at(2),
                  kTestIPAddr.at(3)),
      "-p",
      std::format("{}", kTestPort1),
      "-a",
      std::format("{:#x}.0{}.0{}.0{}", kTestIPAddr.at(0), kTestIPAddr.at(1), kTestIPAddr.at(2),
                  kTestIPAddr.at(3)),
      "-p",
      std::format("{}", kTestPort2),
      "-L",
      kTestLib.begin(),
      "-r",
      kTestRole.begin(),
      "-i",
      std::format("{}", kTestIndex)};

  std::vector<std::array<uint8_t, 4>> _addresses = {kTestIPAddr, kTestIPAddr};
  std::vector<std::string> _libs = {kTestLib.begin()};
  std::vector<size_t> _ports = {kTestPort1, kTestPort2};
};

BENCHMARK_F(ArgvFixture, AddressPortParsingTest)(benchmark::State& st)
{
  size_t i = 0;
  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::vector<std::string> wrapped_input =
          //NOLINTNEXTLINE
          parsing::getInput(const_cast<char**>(_argv.data()), static_cast<int>(_argv.size()));
    }
  }
}

BENCHMARK_F(ParsingFixture, FlagsParsing)(benchmark::State& st)
{
  size_t i = 0;
  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);
    }
  }
}

BENCHMARK_F(ParsingFixture, FlagsParsingWrongFlag)(benchmark::State& st)
{
  size_t i = 0;
  _parsed_data.emplace_back("-M");

  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);
    }
  }
}

BENCHMARK_F(ParsingFixture, FlagsParsingNotPairedFlag)(benchmark::State& st)
{
  size_t i = 0;
  _parsed_data.emplace_back("-L");

  for (auto _ : st) {
    while (i++ < kTestCount) {

      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);
    }
  }
}

BENCHMARK_F(ParsingFixture, WrongPortParsing)(benchmark::State& st)
{
  _parsed_data.emplace_back("-p");
  _parsed_data.emplace_back("-30fds0");

  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);

      auto ports = argv_split->getPorts();
    }
  }
}

BENCHMARK_F(ParsingFixture, WrongIndexParsing)(benchmark::State& st)
{
  _parsed_data.emplace_back("-i");
  _parsed_data.emplace_back("-30fds0");

  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);
    }
  }
}

BENCHMARK_F(ParsingFixture, WrongAddressParsing)(benchmark::State& st)
{
  _parsed_data.emplace_back("-a");
  _parsed_data.emplace_back("127.0.0.test");

  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {

      std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult> argv_split =
          parsing::parseClArgs(_parsed_data);

      auto addresses = argv_split->getAddresses();
    }
  }
}

BENCHMARK_F(InputFixture, OptionsPickTest)(benchmark::State& st)
{
  std::vector<std::string_view> arr{"QUIT",  "EXIT",  "TyPe",     "Vector", "rolE",
                                    "PRINT", "EmpTY", "Settings", "Send",   "Clear"};
  const auto& menu_options = custom_types::getMenuOptions();
  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      std::string str;
      for (const auto& input_str : arr) {
        _cin << input_str;
        _cin >> str;
        std::ranges::transform(str, str.begin(), ::tolower);
        menu_options.contains(std::hash<std::string_view>{}(str));
      }
    }
  }
}

BENCHMARK_F(InputFixture, TypeTest)(benchmark::State& st)
{
  std::vector<std::string_view> types{
      "int",     "float",   "double",  "char",    "string",   "bool",     "int8_t",
      "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
  };

  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      for (auto& type : types) {
        _cin << std::format("Type\n{}\n", type);
        _menu.menuTask(0, 0, _arguments);
      }
    }
  }
}

BENCHMARK_F(InputFixture, RoleTest)(benchmark::State& st)
{
  std::vector<std::string_view> user_names{"User", "Admin", "testrole"};

  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      for (auto& name : user_names) {
        _cin << std::format("Name\n{}\n", name);
        _menu.menuTask(0, 0, _arguments);
      }
    }
  }
}

BENCHMARK_F(InputFixture, VectorTest)(benchmark::State& st)
{
  std::vector<std::string_view> vectors{
      "1 2 3 4\n",
      "   1                   2                    3                   4\n",
      "1 2  3    4 fadsf\n",
      "1 2  3    4 41241241\n",
      "1 2  3    4 41241241ffd\nquit\n",
  };
  size_t i = 0;

  for (auto _ : st) {
    while (i++ < kTestCount) {
      for (auto& vector : vectors) {
        _cin << std::format("Vector\n{}", vector);
        _menu.menuTask(0, 0, _arguments);
      }
    }
  }
}

int main(int argc, char* argv[])
{
  logging::Logger::loggerInit("logs", config::LogVerbosity::NOLOG);
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return 0;
}
