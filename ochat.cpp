
#include <httplib.h>

#include <algorithm>
#include <boost/json/src.hpp> // must include from 1 source file, to eliminate need to link to boost
#include <iostream>
#include <stdexcept> // Include for std::runtime_error
#include <string>
#include <sstream>
#include <vector>

#include "app_config.h"
#include "boost/json.hpp"
#include <boost/json/string.hpp>
using namespace std;

constexpr const char *AI_ENDPOINT = "/api/chat";
constexpr const char *AI_MODEL = "llama3.2:1b";

bool g_debug = ENABLE_DEBUG_LOG;

// returns the object pointer at the specified key or nullptr if the key doesn't
// exist or if the value isn't and object
boost::json::object const *if_contains_object(const boost::json::object &obj,
                                              const string &key)
{
  return obj.contains(key) ? obj.at(key).if_object() : nullptr;
}

// returns the string pointer at the specified key or nullptr if the key doesn't
// exist or if the value isn't and object
boost::json::string const *if_contains_string(const boost::json::object &obj,
                                              const string &key)
{
  return obj.contains(key) ? obj.at(key).if_string() : nullptr;
}

/**
 * Send a request to an Ollama server and retrieve its response.
 * @param req The input text to be sent to the AI server.
 * @return The output text from the AI server, as a string.
 */
string SendRequestToAi(const string &req, vector<string> &history)
{
  std::string aiModel = AI_MODEL;
  boost::json::string prompt(req.c_str(), req.size());
  std::stringstream output;
  std::string endpoint = AI_ENDPOINT;

  // create a connection to the Ollama host
  httplib::Client client(OLLAMA_SERVER_ADDR, OLLAMA_SERVER_PORT);
  client.set_read_timeout(30.0f);

  // format the message containing the AI model and req for Ollama server
  std::stringstream ss;

  ss << "{"
     << "  \"model\": \"" << aiModel << "\","
     << "  \"stream\": false,"
     << " \"messages\": [" << endl;
  for (auto h : history)
  {
    ss << h;
  }
  ss
      << "   { \"role\": \"user\", " << "  \"content\": " << prompt << " }"
      << endl
      << "  ]"
      << "}";
  std::string body = ss.str();
  if (g_debug)
  {
    ::cout << body << endl;
  }

  // Make the API call and get the response
  httplib::Result status = client.Post(endpoint, body, "application/json");

  // check for error sending the post request
  if (status.error() != httplib::Error::Success)
  {
    stringstream err;
    err << COLOR::ATTN << "Error " << status.error() << " when making request to " << OLLAMA_SERVER_ADDR
        << ":" << OLLAMA_SERVER_PORT << COLOR::DEFAULT << endl;
    err << COLOR::ATTN << "- Make sure that the Ollama service is running"
        << COLOR::DEFAULT << endl;
    throw std::runtime_error(err.str());
  }

  // check for error returned by the server
  httplib::Response res = status.value();
  if (res.status != 200)
  {
    stringstream err;
    err << "Error calling API: " << endpoint << ", status code: " << res.status
        << std::endl;
    throw std::runtime_error(err.str());
  }

  // The server responded with success.
  // Parse the returned data, each "content" string contains a chunk of the
  // output from the AI and needs to be combined together into output.
  if (g_debug)
  {
    cout << res.body;
  }
  std::stringstream input;
  input << res.body;
  int line_num = 0;
  std::string line;
  while (std::getline(input, line))
  {
    boost::json::value respLine = boost::json::parse(line);
    if (auto obj = respLine.if_object())
    {
      // parse response for /api/chat
      if (auto msgObj = if_contains_object(*obj, "message"))
      {
        if (auto respChunk = if_contains_string(*msgObj, "content"))
        {
          // converting the boost::json::string to std::string automatically
          // converts the escape sequences (such as \n) appropriately.
          std::string rc = respChunk->c_str();
          output << rc;
          if (g_debug)
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
  newHist << " { \"role\": \"user\", " << "  \"content\": " << prompt << " }," << endl;
  newHist << " { \"role\": \"assistant\", " << "  \"content\": " << lastResponse << " }," << endl;
  history.push_back(newHist.str());

  return ret;
}

int main()
{
  // chat history to preserve context
  vector<string> history;

  /// Initiate loop to handle user input and AI response
  cout << COLOR::APP << "Please enter a prompt for the AI or " << COLOR::WARN << "/help" << COLOR::DEFAULT << " ,for help, " << COLOR::ATTN << "/bye" << COLOR::APP << " , to exit" << COLOR::DEFAULT << endl;
  std::string prompt;
  cout << COLOR::USER << "PROMPT: ";
  while (getline(cin, prompt))
  {

    if (prompt == "/help")
    {
      cout << COLOR::APP;
      cout << "Help" << endl;
      cout << "Enter a question for the AI, the following special commands are supported:" << endl;
      cout << "  /bye - to exit" << endl;
      cout << "  /new - start a new conversation and clear the chat context" << endl;
      cout << "  /debug - to enable debug" << endl;
      cout << "  /help - for this help text" << endl;
      cout << COLOR::DEFAULT;
    }
    else if (prompt == "/new")
    {
      cout << COLOR::APP << "Starting a new conversation, clearing previous chat context" << COLOR::DEFAULT << endl;
      history.clear();
    }
    else if (prompt == "/debug")
    {
      g_debug = !g_debug;
      cout << COLOR::ATTN << "Toggled Debug, g_debug is now " << g_debug << COLOR::DEFAULT << endl;
    }
    else if (prompt == "/bye")
    {
      cout << COLOR::ATTN << "Exiting Chat..." << COLOR::DEFAULT << endl;
      break;
    }
    else
    {
      try
      {
        string resp = SendRequestToAi(prompt, history);
        cout << COLOR::AI << "AI: " << resp << COLOR::DEFAULT << endl;
      }
      catch (const std::runtime_error &e)
      {
        std::cerr << "Exception: " << e.what() << std::endl;
        break;
      }
    }
    cout << COLOR::USER << "PROMPT: ";
  }
}
