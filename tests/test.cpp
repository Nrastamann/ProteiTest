#include <gtest/gtest.h>
#include <unistd.h>
#include <array>
#include <format>
#include <iterator>
#include <string>
#include <vector>
#include "parsing.hpp"
#include "settings.hpp"

class InputFixture : public testing::Test {
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
};

class ArgvFixture : public testing::Test {
 protected:
  static constexpr size_t kArgAmount{39};

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

  std::vector<std::string> _parsed_data = {
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

class SettingsFixture : public testing::Test {
 protected:
};
TEST_F(ArgvFixture, AddressPortParsingTest)
{
  std::vector<std::string> wrapped_input =
      getInput(_argv.data(), static_cast<int>(_argv.size()));

  EXPECT_EQ(wrapped_input.size(), _parsed_data.size());
  auto it_wrapped = _parsed_data.begin();
  for (auto& i : wrapped_input) {
    EXPECT_EQ(i, *it_wrapped);
    std::advance(it_wrapped, 1);
  }
}

TEST_F(ParsingFixture, FlagsParsing)
{
  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

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

  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), false);

  ASSERT_EQ(argv_split.error(), parsing_protei::ParseResult::WRONG_FLAG);
}

TEST_F(ParsingFixture, FlagsParsingNotPairedFlag)
{
  _parsed_data.emplace_back("-L");

  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), false);

  ASSERT_EQ(argv_split.error(), parsing_protei::ParseResult::NO_ARGUMENT);
}

TEST_F(ParsingFixture, WrongPortParsing)
{
  _parsed_data.emplace_back("-p");
  _parsed_data.emplace_back("-30fds0");

  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto ports = argv_split->getPorts();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  ASSERT_EQ(ports.size(), 0);
}

TEST_F(ParsingFixture, WrongIndexParsing)
{
  _parsed_data.emplace_back("-i");
  _parsed_data.emplace_back("-30fds0");

  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto index = argv_split->getIndex();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  EXPECT_EQ(index, 0);
}

TEST_F(ParsingFixture, WrongAddressParsing)
{
  _parsed_data.emplace_back("-a");
  _parsed_data.emplace_back("127.0.0.test");

  std::expected<CommandLineArgsHolder, parsing_protei::ParseResult> argv_split =
      parsing_protei::parseClArgs(_parsed_data);

  ASSERT_EQ(argv_split.has_value(), true);

  auto addresses = argv_split->getAddresses();
  ASSERT_EQ(argv_split.value().parsingStatus(), true);
  EXPECT_EQ(addresses.size(), 0);
}
