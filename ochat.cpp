
#include <httplib.h>

#include <algorithm>
#include <boost/json/src.hpp>  // must include from 1 source file, to eliminate need to link to boost
#include <iostream>
#include <stdexcept>  // Include for std::runtime_error
#include <string>

#include "app_config.h"
#include "boost/json.hpp"
using namespace std;

constexpr const char *AI_ENDPOINT = "/api/chat";
constexpr const char *AI_MODEL = "llama3.2:1b";

bool g_debug = ENABLE_DEBUG_LOG;

// returns the object pointer at the specified key or nullptr if the key doesn't
// exist or if the value isn't and object
boost::json::object const *if_contains_object(const boost::json::object &obj,
                                              const string &key) {
  return obj.contains(key) ? obj.at(key).if_object() : nullptr;
}

// returns the string pointer at the specified key or nullptr if the key doesn't
// exist or if the value isn't and object
boost::json::string const *if_contains_string(const boost::json::object &obj,
                                              const string &key) {
  return obj.contains(key) ? obj.at(key).if_string() : nullptr;
}

/**
 * Send a request to an Ollama server and retrieve its response.
 * @param req The input text to be sent to the AI server.
 * @return The output text from the AI server, as a string.
 */
string SendRequestToAi(const string &req) {
  std::string aiModel = AI_MODEL;
  std::string prompt = req;
  std::stringstream output;
  std::string endpoint = AI_ENDPOINT;

  // create a connection to the Ollama host
  httplib::Client client(OLLAMA_SERVER_ADDR, OLLAMA_SERVER_PORT);

  // format the message containing the AI model and req for Ollama server
  std::stringstream ss;
  ss << "{"
     << "  \"model\": \"" << aiModel << "\","
     << "  \"streaming\": false,"
     << " \"messages\": ["
     << "   { \"role\": \"user\", " << "  \"content\": \"" << prompt << "\" }"
     << "  ]"
     << "}";
  std::string body = ss.str();
  if (g_debug) {
    ::cout << body << endl;
  }

  // Make the API call and get the response
  httplib::Result status = client.Post(endpoint, body, "application/json");

  // check for error sending the post request
  if (status.error() != httplib::Error::Success) {
    stringstream err;
    err << "Error making request to " << OLLAMA_SERVER_ADDR << ":"
        << OLLAMA_SERVER_PORT << endl;
    throw std::runtime_error(err.str());
  }

  // check for error returned by the server
  httplib::Response res = status.value();
  if (res.status != 200) {
    stringstream err;
    err << "Error calling API: " << endpoint << ", status code: " << res.status
        << std::endl;
    throw std::runtime_error(err.str());
  }

  // The server responded with success.
  // Parse the returned data, each "content" string contains a chunk of the
  // output from the AI and needs to be combined together into output.
  if (g_debug) {
    cout << res.body;
  }
  std::stringstream input;
  input << res.body;
  int line_num = 0;
  std::string line;
  while (std::getline(input, line)) {
    boost::json::value respLine = boost::json::parse(line);
    if (auto obj = respLine.if_object()) {
      // parse response for /api/chat
      if (auto msgObj = if_contains_object(*obj, "message")) {
        if (auto respChunk = if_contains_string(*msgObj, "content")) {
          // converting the boost::json::string to std::string automatically
          // converts the escape sequences (such as \n) appropriately.
          std::string rc = respChunk->c_str();
          output << rc;
          if (g_debug) cout << line_num << ": " << *respChunk << endl;
        }
      }
    }
    line_num++;
    line.clear();
  }

  return output.str();
}

int main() {
  // Initial connection and basic intro from AI
  cout << COLOR::APP << "Initiating connection to Ollama model..." << AI_MODEL
       << COLOR::DEFAULT << endl;
  try {
    string resp = SendRequestToAi("Introduce yourself?");
    cout << COLOR::AI << "AI: " << resp << COLOR::DEFAULT << endl;
  } catch (const std::runtime_error &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  /// Initiate loop to handle user input and AI response
  cout << COLOR::APP << "Please enter a prompt for the AI or " << COLOR::ATTN
       << "/bye" << COLOR::APP << " to exit" << COLOR::DEFAULT << endl;
  std::string prompt;
  cout << COLOR::USER << "PROMPT: ";
  while (getline(cin, prompt)) {
    if (prompt == "/bye") {
      cout << COLOR::ATTN << "Exiting Chat..." << COLOR::DEFAULT << endl;
      break;
    } else if (prompt == "/debug") {
      g_debug = !g_debug;
      cout << COLOR::ATTN << "Toggled Debug, g_debug is now " << g_debug;
    } else {
      try {
        string resp = SendRequestToAi(prompt);
        cout << COLOR::AI << "AI: " << resp << COLOR::DEFAULT << endl;
      } catch (const std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        break;
      }
    }
    cout << COLOR::USER << "PROMPT: ";
  }
}
