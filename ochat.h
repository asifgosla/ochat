#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace ochat {

struct Options {
  std::string server;
  int port;
  std::string endpoint;
  std::string model;
  bool stream_resp;
  bool debug;
  std::ostream *ostrm;
};

// Get reference to the options object for the library.
Options &GetOptions();

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

/**
 * Sends an HTTP POST request to the Ollama AI model with the given parameters
 * and updates the conversation history.
 *
 * @param req The formatted HTTP POST request as a string.
 * @param history A vector containing the conversation history, which will be
 * updated with the new message.
 */
void SendRequestToAi(const std::string &req, std::vector<std::string> &history);

// api wrappers for free functions to allow tests to mock the functions
// use weak linkage for these so that the api can be replaced by a mock version
// if running in a test.
class AsioInterface {
public:
  AsioInterface() {}
  virtual ~AsioInterface() {}

  virtual std::size_t read_until(boost::asio::ip::tcp::socket &s,
                                 boost::asio::streambuf &b,
                                 BOOST_ASIO_STRING_VIEW_PARAM delim) = 0;

  virtual std::size_t read_until(boost::asio::ip::tcp::socket &s,
                                 boost::asio::streambuf &b,
                                 BOOST_ASIO_STRING_VIEW_PARAM delim,
                                 boost::system::error_code &ec) = 0;
};

} // namespace ochat
