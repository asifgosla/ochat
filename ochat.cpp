
#include "app_config.h"
#include "boost/json.hpp"
#include <algorithm>
#include <boost/json/src.hpp> // must include from 1 source file, to eliminate need to link to boost
#include <boost/json/string.hpp>
#include <getopt.h>
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <stdexcept> // Include for std::runtime_error
#include <string>
#include <vector>
using namespace std;

struct Options {
  std::string endpoint;
  std::string model;
  bool debug;
};

Options g_opt{OLLAMA_ENDPOINT, OLLAMA_MODEL, ENABLE_DEBUG_LOG};

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
string SendRequestToAi(const string &req, vector<string> &history) {
  boost::json::string prompt(req.c_str(), req.size());
  std::stringstream output;
  std::string endpoint = g_opt.endpoint;

  // create a connection to the Ollama host
  httplib::Client client(OLLAMA_SERVER_ADDR, OLLAMA_SERVER_PORT);
  client.set_read_timeout(30.0f);

  // format the message containing the AI model and req for Ollama server
  std::stringstream ss;

  ss << "{"
     << "  \"model\": \"" << g_opt.model << "\","
     << "  \"stream\": false,"
     << " \"messages\": [" << endl;
  for (auto h : history) {
    ss << h;
  }
  ss << "   { \"role\": \"user\", " << "  \"content\": " << prompt << " }"
     << endl
     << "  ]"
     << "}";
  std::string body = ss.str();
  if (g_opt.debug) {
    ::cout << body << endl;
  }

  // Make the API call and get the response
  httplib::Result status = client.Post(endpoint, body, "application/json");

  // check for error sending the post request
  if (status.error() != httplib::Error::Success) {
    stringstream err;
    err << COLOR::ATTN << "Error " << status.error()
        << " when making request to " << OLLAMA_SERVER_ADDR << ":"
        << OLLAMA_SERVER_PORT << COLOR::DEFAULT << endl;
    err << COLOR::ATTN << "- Make sure that the Ollama service is running"
        << COLOR::DEFAULT << endl;
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
  if (g_opt.debug) {
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
          if (g_opt.debug)
            cout << line_num << ": " << *respChunk << endl;
        }
      }
    }
    line_num++;
    line.clear();
  }

  // save history
  std::string ret = output.str();
  boost::json::string lastResponse(ret.c_str(), ret.size());
  std::stringstream newHist;
  newHist << " { \"role\": \"user\", " << "  \"content\": " << prompt << " },"
          << endl;
  newHist << " { \"role\": \"assistant\", " << "  \"content\": " << lastResponse
          << " }," << endl;
  history.push_back(newHist.str());

  return ret;
}

void show_usage_help() {
  cout << COLOR::APP;
  cout << "Help" << endl;
  cout << "Usage: ochat [options]" << endl;
  cout << "Options:" << endl;
  cout << "  --model=<model> - specify the AI model to use (default: "
       << g_opt.model << ")" << endl;
  cout << "  --help          - display help text" << endl;
  cout << COLOR::DEFAULT;
}

// returns 0 on success, non-zero if failure
int parse_command_line_options(int argc, char **argv, Options &opt) {
  // Define the command-line options
  static struct option long_options[] = {
      {"model", required_argument, nullptr, 'm'},
      {"help", no_argument, nullptr, 'h'},
      {0, 0, 0, 0}};

  /// Parse the command line
  int c;
  while ((c = getopt_long(argc, argv, "m:", long_options, nullptr)) != -1) {
    switch (c) {
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
        string resp = SendRequestToAi(prompt, history);
        cout << COLOR::AI << "AI: " << resp << COLOR::DEFAULT << endl;
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
