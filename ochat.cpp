
#include "app_config.h"
#include "boost/json.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/json/src.hpp> // must include from 1 source file, to eliminate need to link to boost
#include <boost/json/string.hpp>
#include <getopt.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept> // Include for std::runtime_error
#include <string>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

struct Options {
  std::string endpoint;
  std::string model;
  bool debug;
};

Options g_opt{OLLAMA_ENDPOINT, OLLAMA_MODEL, ENABLE_DEBUG_LOG};

// function to return a post request message for the Ollama API
std::string format_post_request(std::string prompt, std::string model_name,
                                bool stream_resp, vector<string> &history) {

  // format the JSON data for the Ollama request
  std::stringstream ss;

  ss << "{"
     << "  \"model\": \"" << g_opt.model << "\","
     << "  \"stream\": " << (stream_resp ? "true" : "false") << ","
     << " \"messages\": [" << endl;
  for (auto h : history) {
    ss << h;
  }
  ss << "   { \"role\": \"user\", " << "  \"content\": \"" << prompt << "\" }"
     << endl
     << "  ]"
     << "}";
  std::string json_data = ss.str();

  // create the post request
  ss.str("");
  ss.clear();
  ss << "POST " << g_opt.endpoint << " HTTP/1.1\r\n";
  ss << "Host: " << OLLAMA_SERVER_ADDR << "\r\n";
  ss << "Content-Type: application/json\r\n";
  ss << "Content-Length: " << json_data.size() << "\r\n";
  ss << "\r\n";

  ss << json_data;
  return ss.str();
}

// Function to parse the HTTP response header
std::map<std::string, std::string>
parse_http_response_header(std::istream &responseStream) {
  std::map<std::string, std::string> headers;
  std::string line;

  // First line is the status line (e.g., HTTP/1.1 200 OK)
  if (std::getline(responseStream, line)) {
    if (line != "\r")
      headers["Status"] = line;
  }

  // Parse each header line
  while (std::getline(responseStream, line)) {
    if (line == "\r")
      break;
    std::string::size_type colon = line.find(':');
    if (colon != std::string::npos) {
      std::string headerName = line.substr(0, colon);
      std::string headerValue = line.substr(colon + 2); // Skip ": "
      headers[headerName] = headerValue;
    }
  }

  return headers;
}

// This function reads data from the socket into resp_buff at least up until the
// delimeter sequence is in the stream.  If there was previously buffered data
// in resp_buff that will be checked for the delimeter before requesting more
// data from the socket.
void read_until_delimeter(tcp::socket &socket,
                          boost::asio::streambuf &resp_buff,
                          const char *delim = "\r\n") {
  if (resp_buff.size() > 0) {
    // check if the data is already buffered up to the next delimeter or if
    // we need to read more.
    std::string residual_str(
        boost::asio::buffer_cast<const char *>(resp_buff.data()),
        resp_buff.size());
    size_t pos = residual_str.find(delim);
    if (pos == std::string::npos) { // no delimiter found, read more
      boost::asio::read_until(socket, resp_buff, delim);
    }
  } else {
    boost::asio::read_until(socket, resp_buff, delim);
  }
}

// Parse the returned JSON data for the content string in the message object.
std::string get_msg_content_from_json(std::string json_str) {
  boost::json::value resp = boost::json::parse(json_str);
  boost::json::object resp_obj = resp.as_object();
  if (resp_obj.find("message") != resp_obj.end()) {
    auto msg_obj = resp_obj["message"].as_object();
    if (msg_obj.find("content") != msg_obj.end()) {
      // converting the boost::json::string to std::string automatically
      // converts the escape sequences (such as \n) appropriately.
      return msg_obj["content"].as_string().c_str();
    }
  }
  return std::string();
}

// Send a request to an Ollama server and display its response.
void SendRequestToAi(const string &req, vector<string> &history) {
  boost::json::string prompt(req.c_str(), req.size());
  std::stringstream output;
  std::string endpoint = g_opt.endpoint;

  // create a connection to the Ollama host
  boost::asio::io_context io_context;
  tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(std::string(OLLAMA_SERVER_ADDR),
                                    STRINGIFY(OLLAMA_SERVER_PORT));
  tcp::socket socket(io_context);
  boost::asio::connect(socket, endpoints);

  // format and send the post request to the ollama server
  bool stream_resp = true;
  std::string post_req =
      format_post_request(req, g_opt.model, stream_resp, history);
  if (g_opt.debug) {
    cout << COLOR::WARN << "POST Request: " << COLOR::DEFAULT << post_req
         << endl;
    ::cout << post_req << endl;
  }
  boost::asio::write(socket, boost::asio::buffer(post_req));

  // read the response header from the server
  std::string myresp;
  boost::asio::streambuf resp_buff;
  resp_buff.prepare(1 << 14); // Prepare buffer to hold up to 16KB of data
  std::istream resp_strm(&resp_buff);
  boost::system::error_code error;
  boost::asio::read_until(socket, resp_buff, "\r\n\r\n", error);

  // parse the response header
  auto res_map = parse_http_response_header(resp_strm);
  if (g_opt.debug) {
    cout << COLOR::WARN << "Parsed response headers: " << COLOR::DEFAULT
         << endl;
    for (auto [k, v] : res_map) {
      std::cout << k << " : " << v << endl;
    }
  }

  // check for chunked transfer or non-chunked w/ content-length specified.
  bool chunked = false;
  int content_length = -1;
  if (res_map.find("Transfer-Encoding") != res_map.end() &&
      res_map["Transfer-Encoding"] == "chunked\r") {
    chunked = true;
    if (g_opt.debug) {
      cout << COLOR::WARN << "Chunked encoding detected" << COLOR::DEFAULT
           << endl;
    }
  } else {
    if (res_map.find("Content-Length") != res_map.end()) {
      content_length = std::stoi(res_map["Content-Length"]);
      if (g_opt.debug) {
        cout << COLOR::WARN << "Content-Length header found: " << content_length
             << COLOR::DEFAULT << endl;
      }
    } else {
      throw std::runtime_error("No Content-Length header found in response");
    }
  }

  std::string resp_body;
  if (chunked) {
    cout << COLOR::AI << "AI: ";
    ;
    // Handle a chunked response:
    // payload of a chunked response starts with the length of the chunk in
    // hexadecimal followed by a \r\n then the actual data of the chunk. then a
    // \r\n indicating end of that chunk.  The final chunk specifies a chunk
    // length of 0;
    while (true) {
      // Read the chunk length followed by "\r\n"
      read_until_delimeter(socket, resp_buff, "\r\n");
      std::string csize_str;
      std::getline(resp_strm, csize_str);
      if (g_opt.debug) {
        cout << COLOR::WARN << "Chunk size: " << csize_str << COLOR::DEFAULT
             << endl;
      }
      size_t cs = std::stoul(csize_str, nullptr, 16);

      if (cs == 0) {
        cout << endl;
        break; // End of chunked responses (this isß the last chunk)
      }

      // Read the chunk data accounting for any previous residual already in the
      // resp_buff.
      size_t residual = resp_buff.size();
      if (residual < cs) {
        size_t cs_remaining = cs - residual;
        boost::asio::read(socket, resp_buff,
                          boost::asio::transfer_exactly(cs_remaining));
      }

      std::string chunk(
          boost::asio::buffer_cast<const char *>(resp_buff.data()), cs);
      resp_buff.consume(chunk.size());
      resp_body += chunk;
      std::string msg = get_msg_content_from_json(chunk);
      output << msg;
      cout << COLOR::AI << msg;
      cout.flush();

      // the trailing "\r\n" marks end of each chunk
      read_until_delimeter(socket, resp_buff, "\r\n");
      std::string chunk_delim_buff(
          boost::asio::buffer_cast<const char *>(resp_buff.data()),
          resp_buff.size());
      size_t chunk_delim_pos = chunk_delim_buff.find("\r\n");
      resp_buff.consume(chunk_delim_pos + 2);
    }
  } else if (content_length > 0) {
    // Handle a response with a Content-Length header
    size_t residual = resp_buff.size();
    if (residual > 0) {
      resp_body = std::string(boost::asio::buffers_begin(resp_buff.data()),
                              boost::asio::buffers_end(resp_buff.data()));
      resp_buff.consume(resp_body.length());
    }
    if ((content_length -= residual) > 0) {

      boost::asio::read(socket, resp_buff,
                        boost::asio::transfer_exactly(content_length));
    }

    resp_body += std::string(boost::asio::buffers_begin(resp_buff.data()),
                             boost::asio::buffers_end(resp_buff.data()));

    cout << COLOR::AI << "AI: " << resp_body << COLOR::DEFAULT << endl;
    // Parse the returned JSON data for the message content.
    output << get_msg_content_from_json(resp_body);
  }

  // save history (to maintain the chat context)
  std::string ret = output.str();
  boost::json::string lastResponse(ret.c_str(), ret.size());
  std::stringstream newHist;
  newHist << " { \"role\": \"user\", " << "  \"content\": " << prompt << " },"
          << endl;
  newHist << " { \"role\": \"assistant\", "
          << "  \"content\": " << lastResponse << " }," << endl;
  history.push_back(newHist.str());
}

void show_usage_help() {
  cout << COLOR::APP;
  cout << "Help" << endl;
  cout << "Usage: ochat [options]" << endl;
  cout << "Options:" << endl;
  cout << "  --debug - enable debug logs" << endl;
  cout << "  --model=<model> - specify the AI model to use (default: "
       << g_opt.model << ")" << endl;
  cout << "  --help          - display help text" << endl;
  cout << COLOR::DEFAULT;
}

// returns 0 on success, non-zero if failure
int parse_command_line_options(int argc, char **argv, Options &opt) {
  // Define the command-line options
  static struct option long_options[] = {
      {"debug", no_argument, nullptr, 'd'},
      {"model", required_argument, nullptr, 'm'},
      {"help", no_argument, nullptr, 'h'},
      {0, 0, 0, 0}};

  /// Parse the command line
  int c;
  while ((c = getopt_long(argc, argv, "m:", long_options, nullptr)) != -1) {
    switch (c) {
    case 'd': // enable debug logs
      opt.debug = true;
      break;
    case 'm':
      opt.model = std::string(optarg);
      cout << COLOR::APP << "Selected Model: " << g_opt.model << COLOR::DEFAULT
           << endl;
      break;
    case 'h':
    default:
      show_usage_help();
      return 1;
    }
  }
  return 0;
}

void show_chat_help() {
  cout << COLOR::APP;
  cout << "Help" << endl;
  cout << "Enter a question for the AI, the following special commands are "
          "supported:"
       << endl;
  cout << "  /bye - to exit" << endl;
  cout << "  /new - start a new conversation and clear the chat context"
       << endl;
  cout << "  /debug - to enable debug" << endl;
  cout << "  /help - for this help text" << endl;
  cout << COLOR::DEFAULT;
}

int main(int argc, char **argv) {
  int ret = parse_command_line_options(argc, argv, g_opt);
  if (ret != 0)
    return ret;

  /// Initiate loop to handle user input and AI response
  vector<string> history; // chat history to preserve context
  cout << COLOR::APP << "Please enter a prompt for the AI or " << COLOR::WARN
       << "/help" << COLOR::DEFAULT << " ,for help, " << COLOR::ATTN << "/bye"
       << COLOR::APP << " , to exit" << COLOR::DEFAULT << endl;
  std::string prompt;
  cout << COLOR::USER << "PROMPT: ";
  while (getline(cin, prompt)) {

    if (prompt == "/help") {
      show_chat_help();
    } else if (prompt == "/new") {
      cout << COLOR::APP
           << "Starting a new conversation, clearing previous chat context"
           << COLOR::DEFAULT << endl;
      history.clear();
    } else if (prompt == "/debug") {
      g_opt.debug = !g_opt.debug;
      cout << COLOR::ATTN << "Toggled Debug, g_opt.debug is now " << g_opt.debug
           << COLOR::DEFAULT << endl;
    } else if (prompt == "/bye") {
      cout << COLOR::ATTN << "Exiting Chat..." << COLOR::DEFAULT << endl;
      break;
    } else {
      try {
        SendRequestToAi(prompt, history);

      } catch (const std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        ret = 1;
        break;
      }
    }
    cout << COLOR::USER << "PROMPT: ";
  }

  return ret;
}
