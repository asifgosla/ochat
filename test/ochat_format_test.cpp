// This file contains unit tests for the ochat module for functions that
// performing formatting and parsing.
//
#include "app_config.h"
#include "ochat.h"
#include "ochat_test_f.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

using OllamaChatTest_F = testing::OllamaChatTest_F;

// Mock an input stream for testing purposes
class FakeInputStream : public std::stringstream {
public:
  FakeInputStream(std::string str) : std::stringstream(str) {}
};

TEST(OchatTest, TestStringify) {
  EXPECT_EQ(std::string(STRINGIFY(314)), std::string("314"));
}

TEST(FormatRequestTest, Basic) {
  // Setup for the function to test
  std::string prompt = "Hello, how are you?";
  std::vector<std::string> history;
  history.push_back(
      " { \"role\": \"user\", \"content\": \"Hi\""
      " { \"role\": \"assistant\", \"content\": \"Hi! How are you?\"");

  std::string body = "{"
                     "  \"model\": \"davinci\","
                     "  \"stream\": false,"
                     " \"messages\": [" +
                     history.front() +
                     "   { \"role\": \"user\", \"content\": \"" + prompt +
                     "\" }"
                     "  ]"
                     "}";
  std::string hdr = "POST /api/chat HTTP/1.1\r\n"
                    "Host: host.docker.internal\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " +
                    std::to_string(body.length()) + "\r\n\r\n";

  std::string expected = hdr + body;

  // call the function to test and check the results
  ochat::Options opt;
  opt.model = "davinci";
  opt.stream_resp = false;
  OllamaChatTest_F oc(opt);

  EXPECT_EQ(oc.FormatPostRequest(prompt, history), expected);
}

TEST(ParseHttpRespHeaderTest, CompleteHttpResponse) {
  std::string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
                         "\r\n";
  FakeInputStream input(response);

  OllamaChatTest_F oc;
  std::map<std::string, std::string> headers = oc.ParseHttpRespHeader(input);

  EXPECT_EQ(headers["Status"], "HTTP/1.1 200 OK\r");
  EXPECT_EQ(headers["Content-Type"], "application/json\r");
  EXPECT_EQ(headers["Date"], "Mon, 27 Jul 2009 12:28:53 GMT\r");
}

TEST(ParseHttpRespHeaderTest, StatusLineWithSpaces) {
  std::string response = "HTTP/1.1 200 OK\r\n"
                         "\r\n";
  FakeInputStream input(response);

  OllamaChatTest_F oc;
  std::map<std::string, std::string> headers = oc.ParseHttpRespHeader(input);

  EXPECT_EQ(headers["Status"], "HTTP/1.1 200 OK\r");
}

TEST(ParseHttpRespHeaderTest, InvalidHeader) {
  std::string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
                         "Invalid-Header\r\n"
                         "\r\n";
  FakeInputStream input(response);

  OllamaChatTest_F oc;
  std::map<std::string, std::string> headers = oc.ParseHttpRespHeader(input);

  EXPECT_EQ(headers["Status"], "HTTP/1.1 200 OK\r");
  EXPECT_EQ(headers["Content-Type"], "application/json\r");
  EXPECT_EQ(headers["Date"], "Mon, 27 Jul 2009 12:28:53 GMT\r");
}

TEST(ParseHttpRespHeaderTest, EmptyResponse) {
  std::string response = "";
  FakeInputStream input(response);

  OllamaChatTest_F oc;
  std::map<std::string, std::string> headers = oc.ParseHttpRespHeader(input);

  EXPECT_TRUE(headers.empty());
}

TEST(ParseHttpRespHeaderTest, NoHeaders) {
  std::string response = "HTTP/1.1 200 OK\r\n\r\n";
  FakeInputStream input(response);

  OllamaChatTest_F oc;
  std::map<std::string, std::string> headers = oc.ParseHttpRespHeader(input);

  EXPECT_EQ(headers["Status"], "HTTP/1.1 200 OK\r");
}

// Test case: JSON with missing content
TEST(GetMsgContentFromJsonTest, MissingContent) {
  std::string json_str = R"(
        {
            "message": {}
        }
    )";

  OllamaChatTest_F oc;
  EXPECT_TRUE(oc.GetMsgContentFromJson(json_str).empty());
}

// Test case: JSON with missing message
TEST(GetMsgContentFromJsonTest, MissingMessage) {
  std::string json_str = R"(
        {
            "other_key": "other_value"
        }
    )";

  OllamaChatTest_F oc;
  EXPECT_TRUE(oc.GetMsgContentFromJson(json_str).empty());
}

// Test case: Empty JSON
TEST(GetMsgContentFromJsonTest, EmptyJson) {
  std::string json_str = "{}";

  OllamaChatTest_F oc;
  EXPECT_TRUE(oc.GetMsgContentFromJson(json_str).empty());
}

// Test case: JSON with invalid format
TEST(GetMsgContentFromJsonTest, InvalidJson) {
  std::string json_str = "invalid_json_string";

  OllamaChatTest_F oc;
  EXPECT_THROW(oc.GetMsgContentFromJson(json_str),
               boost::wrapexcept<boost::system::system_error>);
}