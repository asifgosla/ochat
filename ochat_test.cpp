#include "app_config.h"
#include "ochat.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

TEST(OchatTest, TestStringify) {
  EXPECT_EQ(std::string(STRINGIFY(314)), std::string("314"));
}

TEST(OchatTest, TestFormatRequestBasic) {
  std::string prompt = "Hello, how are you?";
  std::string model_name = "davinci";
  bool stream_resp = false;
  std::vector<std::string> history = {};

  std::string body = "{"
                     "  \"model\": \"davinci\","
                     "  \"stream\": false,"
                     " \"messages\": ["
                     "   { \"role\": \"user\", \"content\": \"" +
                     prompt +
                     "\" }"
                     "  ]"
                     "}";
  std::string hdr = "POST /api/chat HTTP/1.1\r\n"
                    "Host: host.docker.internal\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: " +
                    std::to_string(body.length()) + "\r\n\r\n";

  std::string expected = hdr + body;

  EXPECT_EQ(format_post_request(prompt, model_name, stream_resp, history),
            expected);
}
