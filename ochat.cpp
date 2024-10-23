
#include "app_config.h"
#include "boost/json.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/json/src.hpp> // must include from 1 source file, to eliminate need to link to boost
#include <boost/json/string.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept> // Include for std::runtime_error
#include <string>
#include <vector>

#include "ochat.h"

using namespace std;
using boost::asio::ip::tcp;
using namespace ochat;

// Get library options object reference
Options &ochat::GetOptions() {
  static ochat::Options opt{OLLAMA_ENDPOINT, OLLAMA_MODEL, OLLAMA_STREAM_RESP,
                            ENABLE_DEBUG_LOG};
  return opt;
}

// returns True if Dbg() is on
bool Dbg() { return GetOptions().debug; }

// function to return a post request message for the Ollama API
std::string ochat::FormatPostRequest(std::string prompt,
                                     vector<string> &history) {

  // format the JSON data for the Ollama request
  std::stringstream ss;

  ss << "{"
     << "  \"model\": \"" << GetOptions().model << "\","
     << "  \"stream\": " << (GetOptions().stream_resp ? "true" : "false") << ","
     << " \"messages\": [";
  for (auto h : history) {
    ss << h;
  }
  ss << "   { \"role\": \"user\", " << "\"content\": \"" << prompt << "\" }"
     << "  ]"
     << "}";
  std::string json_data = ss.str();

  // create the post request
  ss.str("");
  ss.clear();
  ss << "POST " << GetOptions().endpoint << " HTTP/1.1\r\n";
  ss << "Host: " << OLLAMA_SERVER_ADDR << "\r\n";
  ss << "Content-Type: application/json\r\n";
  ss << "Content-Length: " << json_data.size() << "\r\n";
  ss << "\r\n";

  ss << json_data;
  return ss.str();
}

// Function to parse the HTTP response header
std::map<std::string, std::string>
ochat::ParseHttpRespHeader(std::istream &responseStream) {
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
      std::string headerValue =
          line.substr(colon + 2, line.size() - 1); // Skip ": "
      headers[headerName] = headerValue;
    }
  }

  return headers;
}

// This function reads data from the socket into resp_buff at least up until the
// delimeter sequence is in the stream.  If there was previously buffered data
// in resp_buff that will be checked for the delimeter before requesting more
// data from the socket.
void ochat::ReadUntilDelimeter(tcp::socket &socket,
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
std::string ochat::GetMsgContentFromJson(std::string json_str) {
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
void ochat::SendRequestToAi(const string &req, vector<string> &history) {
  boost::json::string prompt(req.c_str(), req.size());
  std::stringstream output;
  std::string endpoint = GetOptions().endpoint;

  // create a connection to the Ollama host
  boost::asio::io_context io_context;
  tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(std::string(OLLAMA_SERVER_ADDR),
                                    STRINGIFY(OLLAMA_SERVER_PORT));
  tcp::socket socket(io_context);
  boost::asio::connect(socket, endpoints);

  // format and send the post request to the ollama server
  std::string post_req = FormatPostRequest(req, history);
  if (Dbg()) {
    cout << COL::WRN << "POST Request: " << COL::DEF << post_req << endl;
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
  auto res_map = ParseHttpRespHeader(resp_strm);
  if (Dbg()) {
    cout << COL::WRN << "Parsed response headers: " << COL::DEF << endl;
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
    if (Dbg()) {
      cout << COL::WRN << "Chunked encoding detected" << COL::DEF << endl;
    }
  } else {
    if (res_map.find("Content-Length") != res_map.end()) {
      content_length = std::stoi(res_map["Content-Length"]);
      if (Dbg()) {
        cout << COL::WRN << "Content-Length header found: " << content_length
             << COL::DEF << endl;
      }
    } else {
      throw std::runtime_error("No Content-Length header found in response");
    }
  }

  std::string resp_body;
  if (chunked) {
    cout << COL::AI << "AI: ";
    ;
    // Handle a chunked response:
    // payload of a chunked response starts with the length of the chunk in
    // hexadecimal followed by a \r\n then the actual data of the chunk. then a
    // \r\n indicating end of that chunk.  The final chunk specifies a chunk
    // length of 0;
    while (true) {
      // Read the chunk length followed by "\r\n"
      ReadUntilDelimeter(socket, resp_buff, "\r\n");
      std::string csize_str;
      std::getline(resp_strm, csize_str);
      if (Dbg()) {
        cout << COL::WRN << "Chunk size: " << csize_str << COL::DEF << endl;
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
      std::string msg = GetMsgContentFromJson(chunk);
      output << msg;
      cout << COL::AI << msg;
      cout.flush();

      // the trailing "\r\n" marks end of each chunk
      ReadUntilDelimeter(socket, resp_buff, "\r\n");
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

    cout << COL::AI << "AI: " << resp_body << COL::DEF << endl;
    // Parse the returned JSON data for the message content.
    output << GetMsgContentFromJson(resp_body);
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
