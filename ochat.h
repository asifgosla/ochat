#ifndef __OCHAT_H__
#define __OCHAT_H__

#include "app_config.h"
#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// forward declare test fixture class (needed for friend declaration)
namespace testing {
class OllamaChatTest_F;
}

namespace ochat {

struct Options {
  std::string server;
  int port;
  std::string endpoint;
  std::string model;
  bool stream_resp;
  bool debug;

  // default constructor
  Options()
      : server(OLLAMA_SERVER_ADDR), port(OLLAMA_SERVER_PORT),
        endpoint(OLLAMA_ENDPOINT), model(OLLAMA_MODEL),
        stream_resp(OLLAMA_STREAM_RESP), debug(ENABLE_DEBUG_LOG) {}
};

// Get reference to the options object for the library.
class OllamaChat {
public:
  OllamaChat(const Options opt = Options(), std::ostream &os = std::cout)
      : os_(os), opt_(opt) {}
  ~OllamaChat() {}

  /**
   * Sends an HTTP POST request to the Ollama AI model with the given
   * parameters and updates the conversation history.
   *
   * @param req The formatted HTTP POST request as a string.
   * @param history A vector containing the conversation history, which will
   * be updated with the new message.
   */
  void SendRequestToAi(const std::string &req);

  /**
   * Resets the conversation context.
   */
  void ResetContext();

protected:
  /**
   * Formats a POST request with the given prompt, stream response flag, and
   * history.
   *
   * @param prompt The user's input.
   * @param history A vector containing the conversation history.
   * @return The formatted POST request as a string.
   */
  std::string FormatPostRequest(std::string prompt,
                                std::vector<std::string> &history);

  /**
   * Parses the HTTP response header from the given input stream.
   *
   * @param responseStream The input stream containing the HTTP response.
   * @return A map of key-value pairs representing the content of the response
   * header.
   */
  std::map<std::string, std::string>
  ParseHttpRespHeader(std::istream &responseStream);

  /**
   * Reads data from the socket until a specified delimiter is encountered.
   *
   * @param socket The TCP socket to read from.
   * @param resp_buff A streambuf to store the read data.
   * @param delim The delimiter string to look for.
   */
  void ReadUntilDelimeter(boost::asio::ip::tcp::socket &socket,
                          boost::asio::streambuf &resp_buff, const char *delim);

  /**
   * Extracts the Ollama response message content from a JSON string.
   *
   * @param json_str The JSON string containing the message content.
   * @return A string representing the message content.
   */
  std::string GetMsgContentFromJson(std::string json_str);

  OllamaChat(const OllamaChat &) = delete;
  OllamaChat(OllamaChat &&) = delete;
  OllamaChat &operator=(const OllamaChat &) = delete;

  std::ostream &os_;
  Options opt_;
  std::vector<std::string> history_; // chat history to preserve context

  // test fixture for unit testing
  friend class ::testing::OllamaChatTest_F;
};

} // namespace ochat

#endif //__OCHAT_H__