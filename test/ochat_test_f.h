#ifndef __OCHAT_TEST_F_H__
#define __OCHAT_TEST_F_H__

#ifndef UNIT_TESTING
#error "This file should only be included for unit testing!"
#endif // !UNIT_TESTING

#include "ochat.h"
namespace testing {

// This class is a test proxy for the ochat::OllamaChat class and provides
// access to the protected / private members of ochat::F for unit testing
// purposes only.
class OllamaChatTest_F {
public:
  OllamaChatTest_F(const ochat::Options opt = ochat::Options(),
                   std::ostream &os = std::cout)
      : obj_(opt, os) {}
  ~OllamaChatTest_F() {}

  void SendRequestToAi(const std::string &req) { obj_.SendRequestToAi(req); }

  void ResetContext() { obj_.ResetContext(); }

  std::string FormatPostRequest(std::string prompt,
                                std::vector<std::string> &history) {
    return obj_.FormatPostRequest(prompt, history);
  }

  std::map<std::string, std::string>
  ParseHttpRespHeader(std::istream &responseStream) {
    return obj_.ParseHttpRespHeader(responseStream);
  }

  void ReadUntilDelimeter(boost::asio::ip::tcp::socket &socket,
                          boost::asio::streambuf &resp_buff,
                          const char *delim) {
    obj_.ReadUntilDelimeter(socket, resp_buff, delim);
  }

  std::string GetMsgContentFromJson(std::string json_str) {
    return obj_.GetMsgContentFromJson(json_str);
  }

  std::vector<std::string> &GetHistoryObj() { return obj_.history_; }

  ochat::OllamaChat obj_;
};

} // namespace testing

#endif // !__OCHAT_TEST_F_H__