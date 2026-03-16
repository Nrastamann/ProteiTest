#include <gtest/gtest.h>
#include <unistd.h>
#include <array>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "custom_types.hpp"
#include "data_pool.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"

class InputFixture : public testing::Test {
  static constexpr size_t kIndexTest{42};

 protected:
  void SetUp() override
  {
    _cinBuf = std::cin.rdbuf();
    _coutBuf = std::cout.rdbuf();
    std::cin.rdbuf(_cin.rdbuf());
    std::cout.rdbuf(_cout.rdbuf());
  }

  void TearDown() override
  {
    std::cin.rdbuf(_cinBuf);
    std::cout.rdbuf(_coutBuf);
  }

  std::stringstream _cin;
  std::stringstream _cout;
  std::streambuf* _cinBuf;
  std::streambuf* _coutBuf;

  Menu _menu;
  AppSettings _command_line_options{{}, {}, {}, "", kIndexTest};
  data_storage::DataPool _data_pool;
  FunctionArgs _arguments{_command_line_options, _data_pool};
};

class ArgvFixture : public testing::Test {
 protected:
  std::vector<char*> _argv = {"./build/Debug/proteip",
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

  std::vector<std::string_view> _parsed_data = {
      "-a", "ff.00.ff.ff", "-p", "3000",
      "-a", "127.0.0.1",   "-a", "122.12.122.122",
      "-p", "5000",        "-a", "127.127.127.127",
      "-p", "2228",        "-a", "127.127.127.127",
      "-p", "2229",        "-a", "127.127.127.127",
      "-p", "2230",        "-a", "127.127.127.127",
      "-p", "2231",        "-i", "12"};
};

class ParsingFixture : public testing::Test {
 protected:
  static constexpr std::array<uint8_t, 4> kTestIPAddr{127, 0, 0, 1};
  static constexpr size_t kTestPort1{3000};
  static constexpr size_t kTestPort2{3001};
  static constexpr size_t kTestIndex{42};
  static constexpr std::string_view kTestRole{"User"};
  static constexpr std::string_view kTestLib{"src"};

  std::vector<std::string> _parsed_data = {
      "-a",
      std::format("{}.0{}.0{}.0{}", kTestIPAddr.at(0), kTestIPAddr.at(1),
                  kTestIPAddr.at(2), kTestIPAddr.at(3)),
      "-p",
      std::format("{}", kTestPort1),
      "-a",
      std::format("{:#x}.0{}.0{}.0{}", kTestIPAddr.at(0), kTestIPAddr.at(1),
                  kTestIPAddr.at(2), kTestIPAddr.at(3)),
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

class ResourceFixture : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(ArgvFixture, AddressPortParsingTest)
{
  std::vector<std::string> wrapped_input =
      parsing::getInput(_argv.data(), static_cast<int>(_argv.size()));

  EXPECT_EQ(wrapped_input.size(), _parsed_data.size());
  auto it_wrapped = _parsed_data.begin();
  for (auto& i : wrapped_input) {
    EXPECT_EQ(i, *it_wrapped);
    std::advance(it_wrapped, 1);
  }
}

TEST_F(ParsingFixture, FlagsParsing)
{
  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  EXPECT_EQ(argv_split.value().parsingStatus(), false);
  EXPECT_EQ(argv_split.value().getRole(), kTestRole);
  EXPECT_EQ(argv_split.value().getIndex(), kTestIndex);
  auto ports = argv_split.value().getPorts();
  auto libs = argv_split.value().getLibs();
  auto addrs = argv_split.value().getAddresses();

  auto it_ports = ports.begin();
  EXPECT_EQ(*it_ports, kTestPort1);

  std::advance(it_ports, 1);
  EXPECT_EQ(*it_ports, kTestPort2);

  EXPECT_EQ(*libs.begin(), kTestLib);
}

TEST_F(ParsingFixture, FlagsParsingWrongFlag)
{
  _parsed_data.emplace_back("-M");

  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), false);

  ASSERT_EQ(argv_split.error(), parsing::ParseResult::WRONG_FLAG);
}

TEST_F(ParsingFixture, FlagsParsingNotPairedFlag)
{
  _parsed_data.emplace_back("-L");

  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), false);

  ASSERT_EQ(argv_split.error(), parsing::ParseResult::NO_ARGUMENT);
}

TEST_F(ParsingFixture, WrongPortParsing)
{
  _parsed_data.emplace_back("-p");
  _parsed_data.emplace_back("-30fds0");

  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto ports = argv_split->getPorts();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  ASSERT_EQ(ports.size(), 0);
}

TEST_F(ParsingFixture, WrongIndexParsing)
{
  _parsed_data.emplace_back("-i");
  _parsed_data.emplace_back("-30fds0");

  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto index = argv_split->getIndex();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  EXPECT_EQ(index, 0);
}

TEST_F(ParsingFixture, WrongAddressParsing)
{
  _parsed_data.emplace_back("-a");
  _parsed_data.emplace_back("127.0.0.test");

  std::expected<parsing::CommandLineArgsHolder, parsing::ParseResult>
      argv_split = parsing::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto addresses = argv_split->getAddresses();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  EXPECT_EQ(addresses.size(), 0);
}

TEST_F(InputFixture, OptionsPickTest)
{
  std::vector<std::string_view> arr{
      "QUIT", "EXIT", "TyPe", "Vector", "rolE", "PRINT", "EmpTY", "Settings",
  };
  const auto& menu_options = custom_types::getMenuOptions();

  EXPECT_EQ(arr.size(), menu_options.size());

  std::string str;
  for (const auto& input_str : arr) {
    _cin << input_str;

    _cin >> str;
    std::ranges::transform(str, str.begin(), ::tolower);

    if (arr.begin() + static_cast<int64_t>(arr.size() - 1) == arr.end()) {
      EXPECT_FALSE(menu_options.contains(std::hash<std::string_view>{}(str)));
      break;
    }

    EXPECT_TRUE(menu_options.contains(std::hash<std::string_view>{}(str)));
  }
}

TEST_F(InputFixture, QuitTest)
{
  EXPECT_FALSE(_command_line_options.cgetShouldClose());

  _cin.str("quit");
  _menu.menuTask(0, 0, _arguments);

  EXPECT_TRUE(_command_line_options.cgetShouldClose());
}

TEST_F(InputFixture, ExitTest)
{
  EXPECT_FALSE(_command_line_options.cgetShouldClose());

  _cin.str("ExIT");
  _menu.menuTask(0, 0, _arguments);

  EXPECT_TRUE(_command_line_options.cgetShouldClose());
}

TEST_F(InputFixture, TypeTest)
{
  std::vector<std::string_view> types{
      "int",     "float",    "double",   "char",     "string",
      "bool",    "int8_t",   "int16_t",  "int32_t",  "int64_t",
      "uint8_t", "uint16_t", "uint32_t", "uint64_t",
  };
  EXPECT_EQ(types.size(), custom_types::getHashToTypeInfo().size());

  for (auto& type : types) {
    _cin << std::format("Type\n{}\n", type);
    _menu.menuTask(0, 0, _arguments);
    EXPECT_EQ(std::hash<std::string_view>{}(type),
              _command_line_options.cgetTypeHash());
  }

  _cin << "Type";
  _cin << "SomethingWrong";
  _cin << "quit";

  _menu.menuTask(0, 0, _arguments);

  EXPECT_NE(std::hash<std::string_view>{}("SomethingWrong"),
            _command_line_options.cgetTypeHash());
}

TEST_F(InputFixture, RoleTest)
{
  std::vector<std::string_view> user_names{"User", "Admin", "testrole"};

  for (auto& name : user_names) {
    _cin << std::format("Name\n{}\n", name);
    _menu.menuTask(0, 0, _arguments);

    EXPECT_EQ(name, _command_line_options.cgetName());
  }
}

TEST_F(InputFixture, VectorTest)
{
  std::array<int, 4> test_arr{1, 2, 3, 4};
  std::vector<std::string_view> vectors{
      "1 2 3 4\n",
      "   1                   2                    3                   4\n",
      "1 2  3    4 fadsf\n",
      "1 2  3    4 41241241\n",
      "1 2  3    4 41241241ffd\nquit\n",
  };

  for (auto& vector : vectors) {
    _cin << std::format("Vector\n{}", vector);
    _menu.menuTask(0, 0, _arguments);
  }

  while (_data_pool.size() != 0) {
    auto* it_test = test_arr.begin();

    for (auto& i : _data_pool.front()._vec) {
      int test = std::get<int>(i);
      EXPECT_EQ(test, *it_test);
      std::advance(it_test, 1);
    }

    _data_pool.pop();
  }
}

TEST_F(InputFixture, VectorTestMultiple)
{

  std::vector<
      std::array<custom_types::any_type, network_addr::kIpAddrOctetAmount>>
      arr{
          {-1, -2, -3, -4},
          {"true", "false", 0, 1},
          {"test", "string", "a", "b"},
          {1, 2, 3, 4},
      };

  std::vector<std::string_view> types{"int8_t\n", "bool\n", "string\n",
                                      "uint8_t\n"};

  std::vector<std::string> input;
  std::vector<std::string_view> input_correct{
      "-1 -2 -3 -4 \n", "1 0 0 1 \n", "test string a b \n", "1 2 3 4 \n"};

  for (auto& i : arr) {
    std::string str;
    for (auto& elem : i) {
      str += std::visit(
          [](const auto& elem) { return std::format("{} ", elem); }, elem);
    }
    input.push_back(str + '\n');
  }

  for (size_t i = 0; i < arr.size(); ++i) {
    _cin << std::format("Type\n{}", types[i]);
    _menu.menuTask(0, 0, _arguments);
    _cin << std::format("Vector\n{}", input[i]);
    _menu.menuTask(0, 0, _arguments);
  }

  auto correct_it = input_correct.begin();

  while (_data_pool.size() != 0) {
    _cout.str(std::string());
    menu_functions::printVector(_data_pool, {});
    _data_pool.pop();

    std::string string_to_check = _cout.str();
    EXPECT_EQ(string_to_check, *correct_it);

    std::advance(correct_it, 1);
  }
}

TEST_F(InputFixture, EmptyVector)
{
  std::array<int8_t, 4> test_arr{1, 2, 3, 4};
  std::vector<std::string_view> vectors{
      "1 2 3 4\n",
      "   1                   2                    3                   4\n",
      "1 2  3    4 fadsf\n",
  };

  _cin << "Type\nint8_t\n";
  _menu.menuTask(0, 0, _arguments);
  for (auto& vector : vectors) {
    _cin << std::format("Vector\n{}", vector);
    _menu.menuTask(0, 0, _arguments);
  }

  _cout.str(std::string());
  _cout.flush();
  _cin << "Empty\n";
  _menu.menuTask(0, 0, _arguments);

  std::string correct_string =
      std::format("{} {} {} {}  - int8_t\n", test_arr[0], test_arr[1],
                  test_arr[2], test_arr[3]);

  std::string result =
      std::format("Your command: \n{}{}{}Queue is empty\n", correct_string,
                  correct_string, correct_string, correct_string);

  EXPECT_EQ(result, _cout.str());
}

TEST_F(InputFixture, PrintSettings)
{
  _cin << "settings\n";
  _menu.menuTask(0, 0, _arguments);

  std::string_view result =
      "Your command: \n========================\nCurrent "
      "settings:\nUserName:\tUserName\nRole:\t\t\nIndex:\t\t42\nIP "
      "address:\n\nLibrary names:\nCurrent "
      "type:\tint\n========================\n";

  EXPECT_EQ(result, _cout.str());
}
int main(int argc, char** argv)
{
  logging::Logger::loggerInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
