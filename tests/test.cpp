#include <gtest/gtest.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "custom_types.hpp"
#include "data_pool.hpp"
#include "ip_addr.hpp"
#include "menu.hpp"
#include "menu_functions.hpp"
#include "parsing.hpp"
#include "server.hpp"
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
    _args.pushIndex("42");
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
  parsing::ArgHolder _args;
  data_storage::DataPool _data_pool;
  AppSettings _command_line_options{_args};
  FunctionArgs _arguments{_command_line_options, _data_pool};
};

class ArgvFixture : public testing::Test {
 protected:
  std::vector<std::string_view> _argv_temp = {"./build/Debug/proteip",
                                              "-a",
                                              "ff",
                                              "00",
                                              "ff",
                                              "ff:3000",
                                              "-a",
                                              "127",
                                              "0",
                                              "0",
                                              "1:3001",
                                              "-a",
                                              "127iuuuy0uijk0l1k_tsts5000",
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
  void SetUp() override
  {
    [this](std::string& str) {
      for (auto& str_sv : _argv_temp) {
        str += str_sv;
        str += '\0';
      }
    }(_result);
    auto it = _result.begin();

    for (auto& str : _argv_temp) {
      _argv.push_back(&*it);
      std::advance(it, str.size() + 1);
    }
  }

  std::string _result;
  std::vector<char*> _argv;
  std::vector<network_addr::IpAddr> _parsed_data_addr = {
      {{0xff, 00, 0xff, 0xff}, 0x3000}, {{127, 0, 0, 1}, 3001},
      {{127, 0, 0, 1}, 5000},           {{127, 127, 127, 127}, 2228},
      {{127, 127, 127, 127}, 2229},     {{127, 127, 127, 127}, 2230},
      {{127, 127, 127, 127}, 2231}};

  size_t index_parsed = 12;
};

class ParsingFixture : public testing::Test {
 protected:
  static constexpr std::array<uint8_t, 4> kTestIPAddr{127, 0, 0, 1};
  static constexpr size_t kTestPort1{3000};
  static constexpr size_t kTestPort2{3001};
  static constexpr size_t kTestIndex{42};
  static constexpr std::string_view kTestRole{"User"};
  static constexpr std::string_view kTestLib{"src"};

  std::vector<char*> _parsed_data = {
      "-a", "127.0.0.1 3000", "-a", "127 0 0 1 3001", "-L", "s", "r", "c",
      "-i", "4poweqip",       "2"};

  std::vector<std::array<uint8_t, 4>> _addresses = {kTestIPAddr, kTestIPAddr};
  std::vector<std::string> _libs = {kTestLib.begin()};
};

class ClientServerFixture : public testing::Test {
 protected:
  std::vector<char*> _argv = {"-a", "127.0.0.1:5000"};

  std::vector<std::string_view> _parsed_data = {"-a", "127.127.127.127", "-p", "2231"};
  std::string_view _server_argv = "5000";
};

TEST_F(ArgvFixture, AddressPortParsingTestPRT)
{
  auto args = parsing::parseArguments(static_cast<int>(_argv.size()), &*_argv.begin(),
                                      parsing::getArgSetterMain());

  ASSERT_TRUE(args.has_value());

  auto addresses = args->getAddr();
  auto it_wrapped = addresses.begin();
  for (auto& i : _parsed_data_addr) {
    EXPECT_EQ(i._addr, it_wrapped->_addr);
    EXPECT_EQ(i._port, it_wrapped->_port);
    std::advance(it_wrapped, 1);
  }
  EXPECT_EQ(index_parsed, args->getIndex());
}
/*
TEST_F(ParsingFixture, FlagsParsingWrongFlagTestPRT)
{
  _parsed_data[_parsed_data.size() - 2] = "-n";

  auto args = parsing::parseArguments(static_cast<int>(_parsed_data.size()),
                                      _parsed_data.data(), parsing::getArgSetterMain());

  ASSERT_FALSE(args.has_value());

  ASSERT_EQ(args.error(), parsing::ParseResult::WRONG_FLAG);
}

TEST_F(ParsingFixture, FlagsParsingNotPairedFlagTestPRT)
{
  _parsed_data.push_back("-L");

  auto args = parsing::parseArguments(static_cast<int>(_parsed_data.size()),
                                      _parsed_data.data(), parsing::getArgSetterMain());

  ASSERT_EQ(args.error(), parsing::ParseResult::NO_ARGUMENT);
}

TEST_F(ParsingFixture, WrongAddressParsingTestPRT)
{
  _parsed_data.push_back("-a");
  _parsed_data.push_back("127.0.0.test");

  auto args = parsing::parseArguments(static_cast<int>(_parsed_data.size()),
                                      _parsed_data.data(), parsing::getArgSetterMain());

  EXPECT_EQ(args.has_value(), false);
}
*/
TEST_F(InputFixture, OptionsPickTestPRT)
{
  std::vector<std::string_view> arr{"QUIT",  "EXIT",  "TyPe",     "Vector", "rolE",
                                    "PRINT", "EmpTY", "Settings", "Send",   "Clear"};

  std::string str;
  for (const auto& input_str : arr) {
    _cin << input_str;

    _cin >> str;
    _cin << "quit";
    std::ranges::transform(str, str.begin(), ::tolower);
    _menu.callFunction(std::hash<std::string_view>{}(str), 0, 0, _arguments);

    EXPECT_FALSE(_cout.str().contains("Wrong input"));
  }
}

TEST_F(InputFixture, QuitTestPRT)
{
  EXPECT_FALSE(_command_line_options.cgetShouldClose());

  _cin.str("quit");
  _menu.menuTask(0, 0, _arguments);

  EXPECT_TRUE(_command_line_options.cgetShouldClose());
}

TEST_F(InputFixture, ExitTestPRT)
{
  EXPECT_FALSE(_command_line_options.cgetShouldClose());

  _cin.str("ExIT");
  _menu.menuTask(0, 0, _arguments);

  EXPECT_TRUE(_command_line_options.cgetShouldClose());
}

TEST_F(InputFixture, TypeTestPRT)
{
  std::vector<std::string_view> types{
      "int",     "float",   "double",  "char",    "string",   "bool",     "int8_t",
      "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
  };

  for (auto& type : types) {
    _cin << std::format("Type\n{}\n", type);
    _menu.menuTask(0, 0, _arguments);
    EXPECT_EQ(std::hash<std::string_view>{}(type), _command_line_options.cgetTypeHash());
  }

  _cin << "Type";
  _cin << "SomethingWrong";
  _cin << "quit";

  _menu.menuTask(0, 0, _arguments);

  EXPECT_NE(std::hash<std::string_view>{}("SomethingWrong"),
            _command_line_options.cgetTypeHash());
}

TEST_F(InputFixture, RoleTestPRT)
{
  std::vector<std::string_view> user_names{"User", "Admin", "testrole"};

  for (auto& name : user_names) {
    _cin << std::format("Name\n{}\n", name);
    _menu.menuTask(0, 0, _arguments);

    EXPECT_EQ(name, _command_line_options.cgetName());
  }
}

TEST_F(InputFixture, VectorTestPRT)
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

TEST_F(InputFixture, VectorTestMultipleTestPRT)
{

  std::vector<std::array<custom_types::any_type, network_addr::kIpAddrOctetAmount>> arr{
      {-1, -2, -3, -4},
      {"true", "false", 0, 1},
      {"test", "string", "a", "b"},
      {1, 2, 3, 4},
  };

  std::vector<std::string_view> types{"int8_t\n", "bool\n", "string\n", "uint8_t\n"};

  std::vector<std::string> input;
  std::vector<std::string_view> input_correct{"-1 -2 -3 -4 \n", "1 0 0 1 \n",
                                              "test string a b \n", "1 2 3 4 \n"};

  for (auto& i : arr) {
    std::string str;
    for (auto& elem : i) {
      str += std::visit([](const auto& elem) { return std::format("{} ", elem); }, elem);
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

TEST_F(InputFixture, EmptyVectorTestPRT)
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

  std::string correct_string = std::format("{} {} {} {}  - int8_t\n", test_arr[0], test_arr[1],
                                           test_arr[2], test_arr[3]);

  std::string result = std::format("Your command: \n{}{}{}Queue is empty\n", correct_string,
                                   correct_string, correct_string, correct_string);

  EXPECT_EQ(result, _cout.str());
}

TEST_F(InputFixture, PrintSettingsTestPRT)
{
  _cin << "settings\n";
  _menu.menuTask(0, 0, _arguments);

  std::string_view result =
      "Your command: \n========================\nCurrent "
      "settings:\nUserName:\tUserName\nRole:\t\tUser\nIndex:\t\t0\nIP "
      "address:\n\nLibrary names:\nCurrent "
      "type:\tint32_t\n========================\n";

  EXPECT_EQ(result, _cout.str());
}

TEST_F(ClientServerFixture, TestServer)
{
  custom_types::PolymorphicVectorQuad vec{1, 2, 3, 4};
  constexpr std::array<double, 4> kVecD{2., -1., 3., 0.25};
  constexpr std::array<bool, 4> kVecBool{true, false, true, false};
  constexpr std::array<std::string_view, 4> kVecStr{"TEST", "ANOTHER", "ONE", "TEST"};

  std::string str;

  server::dataManipulation(str, vec);

  EXPECT_EQ(str, "2 -1 3 0 ");
  vec = {1., 2., 3., 4.};

  server::dataManipulation(str, vec);

  for (size_t i = 0; i < vec.size(); ++i) {
    EXPECT_DOUBLE_EQ(std::get<double>(vec.at(i)), kVecD.at(i));
  }

  vec = {true, false, false, true};

  server::dataManipulation(str, vec);

  for (size_t i = 0; i < vec.size(); ++i) {
    EXPECT_DOUBLE_EQ(std::get<bool>(vec.at(i)), kVecBool.at(i));
  }

  vec = {"test", "another", "one", "test"};

  server::dataManipulation(str, vec);

  for (size_t i = 0; i < vec.size(); ++i) {
    EXPECT_EQ(std::get<std::string>(vec.at(i)), kVecStr.at(i));
  }
}
/*
 * TODO: Somehow need to test this inside gtest
TEST_F(ClientServerFixture, TestServerThreads)
{
  custom_types::PolymorphicVectorQuad result{32, 9, 3072, 0};

  constexpr size_t kTransformationNumber{5};
  constexpr size_t kNumberClient{10};

  parsing::ArgHolder args;
  args.pushAddr("127.0.0.1:5000");

  thread_pool::ThreadPool threads{kNumberClient};
  AppSettings settings(args);
  std::array<char*, 2> argv_temp = {"1", "127.0.0.1:5000"};
  std::thread server_worker(server::serverStart, 2, argv_temp.data());

  server_worker.detach();

  auto task = [](AppSettings& settings, const custom_types::PolymorphicVectorQuad& result) {
    std::stringstream cin{};
    std::stringstream cout{};
    std::streambuf* cin_buf{};
    std::streambuf* cout_buf{};

    cin_buf = std::cin.rdbuf();
    cout_buf = std::cout.rdbuf();
    std::cin.rdbuf(cin.rdbuf());
    std::cout.rdbuf(cout.rdbuf());

    std::cin.rdbuf(cin_buf);
    std::cout.rdbuf(cout_buf);

    Menu menu;
    data_storage::DataPool data_pool;
    FunctionArgs arguments(settings, data_pool);

    cin << "Vector\n1 2 3 4\n"; 
    menu.menuTask(0, 0, arguments);

    for (size_t j = 0; j < kTransformationNumber; ++j) {
      cin << "Send\n";
      menu.menuTask(0, 0, arguments);
    }

    EXPECT_EQ(data_pool.size(), 1);
    const auto* it = std::prev(result.begin());
    EXPECT_TRUE(std::ranges::all_of(data_pool.front()._vec, [it](const auto& element) {
      return std::get<int>(*std::next(it)) == std::get<int>(element);
    }));

    cin << "quit\n";
    menu.menuTask(0, 0, arguments);
  };

  for (size_t i = 0; i < kNumberClient; ++i) {
    threads.addTask(task, std::ref(settings), std::cref(result));
  }

  threads.waitAll();

  close(getSetServerSocket(server::GetSocketN{}));
}
*/
int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
