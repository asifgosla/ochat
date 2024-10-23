#include "app_config.h"
#include "mock_asio.h"
#include "ochat.h"
#include <gmock/gmock.h> // Brings in gMock.
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
using namespace testing;

// Fake boost streambuf for testing purposes
class FakeBBuf : public boost::asio::streambuf {
public:
  FakeBBuf(string str) {}
};

MockAsio *g_mock_asio = nullptr;

class OchatAsioTest : public ::testing::Test {
protected:
  OchatAsioTest() { g_mock_asio = &asio_; }
  ~OchatAsioTest() { g_mock_asio = nullptr; };

  MockAsio asio_;
};

MATCHER(BufIsEmpty, "Checks if the buffer's size is 0") {
  return arg.size() == 0;
}

MATCHER_P(BufIsEqualTo, value, "Checks the buffer's contents") {
  std::string str(static_cast<char *>(arg.data()), arg.size());
  return str == value;
}

TEST(ReadUntilDelimeterTest, CheckValidDelim) {
  try {
    boost::asio::io_context io_context;
    BSocket socket(io_context);
    FakeBBuf buff("");
    MockAsio mock_asio;
    g_mock_asio = &mock_asio;

    // Set up the expectation for the asio call.
    std::string resp{
        "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!"};
    EXPECT_CALL(mock_asio, read_until(_, BufIsEmpty(), _))
        .WillOnce([&buff, resp](BSocket & /*s*/, BStreamBuf &b,
                                string_view /*delim*/) {
          // Write data into  buffer to simulate reading from socket
          std::ostream os(&b);
          os << resp;
          return resp.size(); // Return the number of bytes written
        });
    ochat::ReadUntilDelimeter(socket, buff, "\r\n");
    std::istream rs(&buff);
    std::string resp_line;
    std::getline(rs, resp_line);
    EXPECT_EQ(resp_line, "HTTP/1.1 200 OK\r");
  } catch (...) {
    FAIL() << "Exception Failure" << endl;
  }
}

TEST(SendRequestToAiTest, ContentLength) {

  try {
    // setup for call
    ochat::Options &opt = ochat::GetOptions();
    opt.stream_resp = false;
    opt.debug = true;
    opt.server = "localhost";
    opt.port = 8000;
    std::stringstream ss;
    opt.ostrm = &ss;
    vector<string> history;
    std::string req = "Hi!";
    FakeBBuf buff("");
    MockAsio mock_asio;
    g_mock_asio = &mock_asio;

    std::string resp{"HTTP/1.1 200 OK\r\nContent-Length: "
                     "13\r\n\r\n{\"message\":{\"content\":\"Hello, World!\"}}"};
    std::string expected_post =
        "POST /api/chat HTTP/1.1\r\nHost: localhost\r\nContent-Type: "
        "application/json\r\nContent-Length: 97\r\n\r\n{  \"model\": "
        "\"davinci\",  \"stream\": false, \"messages\": [   { \"role\": "
        "\"user\", \"content\": \"Hi!\" }  ]}";

    EXPECT_CALL(mock_asio, write(_, BufIsEqualTo(expected_post)))
        .WillOnce([](BSyncWrStream &s, const BConstBufSeqType &b) {
          return b.size();
        });
    EXPECT_CALL(mock_asio, read_until(_, BufIsEmpty(), _))
        .WillOnce([&buff, resp](BSocket & /*s*/, BStreamBuf &b,
                                string_view /*delim*/) {
          // Write data into  buffer to simulate reading from socket
          std::ostream os(&b);
          os << resp;
          return resp.size(); // Return the number of bytes written
        });
    ochat::SendRequestToAi(req, history);

  } catch (...) {
    FAIL() << "Exception Failure" << endl;
  }
}

TEST(SendRequestToAiTest, ChunkedResp) {
  try {
    // setup for call
    ochat::Options &opt = ochat::GetOptions();
    opt.debug = true;
    opt.server = "localhost";
    opt.port = 8000;
    opt.stream_resp = true;
    std::stringstream ss;
    opt.ostrm = &ss;
    vector<string> history;
    std::string req = "Hi!";
    FakeBBuf buff("");
    MockAsio mock_asio;
    g_mock_asio = &mock_asio;

    // Note that the response is split into multiple chunks of data.
    // the chunk boundaries are chosen to excercise the different code paths in
    // the api being tested.
    vector<string> resp{
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n20",
        "\r\n{\"message\"", ":{\"content\":\"Hello,\"}}",
        "\r\n21\r\n{\"message\":{\"content\":\" World!\"}}", "\r\n0\r\n"};
    auto itResp = resp.begin();
    // Expect call to write (but don't verify the output as part of this test)
    EXPECT_CALL(mock_asio, write(_, _))
        .WillOnce([](BSyncWrStream &s, const BConstBufSeqType &b) {
          return b.size();
        });
    EXPECT_CALL(mock_asio, read(_, _, _))
        .WillOnce(
            // simulate reading from socket
            [&itResp](BSyncRdStream &s, BStreamBuf &b, BCompletionCond cc) {
              std::ostream os(&b);
              size_t resp_size = itResp->size();
              os << *itResp++;
              return resp_size;
              // todo: how to validate paraemter (check delim = \r\n\r\n first
              // time and then \r\n after that)
            });
    EXPECT_CALL(mock_asio, read_until(_, _, _))
        .Times(resp.size() - 1)
        .WillRepeatedly(
            [&itResp](BSocket & /*s*/, BStreamBuf &b, string_view /*delim*/) {
              // Write data into  buffer to simulate reading from socket
              std::ostream os(&b);
              size_t resp_size = itResp->size();
              os << *itResp++;
              return resp_size;
              // todo: how to validate paraemter (check delim = \r\n\r\n first
              // time and then \r\n after that)
            });

    // call the api being tested
    ochat::SendRequestToAi(req, history);

  } catch (...) {
    FAIL() << "Exception Failure" << endl;
  }
}
// add error case, not chunked and no Content-Length (throws
// std::runtime_error("No Content-Length header found in response"))