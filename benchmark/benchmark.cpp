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
#include "server.hpp"
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
  parsing::ArgHolder _args;
  AppSettings _command_line_options{_args};
  data_storage::DataPool _data_pool;
  FunctionArgs _arguments{_command_line_options, _data_pool};
};

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

BENCHMARK_F(InputFixture, ComputeTest)(benchmark::State& st)
{
  size_t i = 0;
  std::array<custom_types::PolymorphicVectorQuad, 4> vecs{{{1, 2, 3, 4},
                                                           {"test1", "test2", "test3", "test4"},
                                                           {false, true, true, false},
                                                           {1., 2., 3., 4.}}};
  std::string result;
  result.reserve(4096);
  for (auto _ : st) {
    while (i++ < kTestCount) {
      for (auto& vector : vecs) {
        server::dataManipulation(result, vector);
      }
    }
  }
}

int main(int argc, char* argv[])
{
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return 0;
}
