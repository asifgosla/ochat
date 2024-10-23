#include "app_config.h"
#include "ochat.h"
#include <getopt.h>
#include <iostream>
#include <string>

using namespace std;

void show_usage_help(ochat::Options &opt) {
  cout << COL::APP;
  cout << "Help" << endl;
  cout << "Usage: ochat [options]" << endl;
  cout << "Options:" << endl;
  cout << "  --debug - enable debug logs" << endl;
  cout << "  --model=<model> - specify the AI model to use (default: "
       << opt.model << ")" << endl;
  cout << "  --help          - display help text" << endl;
  cout << COL::DEF;
}

// returns 0 on success, non-zero if failure
int ParseOptions(int argc, char **argv, ochat::Options &opt) {
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
      cout << COL::APP << "Selected Model: " << opt.model << COL::DEF << endl;
      break;
    case 'h':
    default:
      show_usage_help(opt);
      return 1;
    }
  }
  return 0;
}

void show_chat_help() {
  cout << COL::APP;
  cout << "Help" << endl;
  cout << "Enter a question for the AI, the following special commands are "
          "supported:"
       << endl;
  cout << "  /bye - to exit" << endl;
  cout << "  /new - start a new conversation and clear the chat context"
       << endl;
  cout << "  /debug - to enable debug" << endl;
  cout << "  /help - for this help text" << endl;
  cout << COL::DEF;
}

int main(int argc, char **argv) {
  ochat::Options &opt = ochat::GetOptions();
  int ret = ParseOptions(argc, argv, opt);
  if (ret != 0)
    return ret;

  /// Initiate loop to handle user input and AI response
  vector<string> history; // chat history to preserve context
  cout << COL::APP << "Please enter a prompt for the AI or " << COL::WRN
       << "/help" << COL::DEF << " ,for help, " << COL::ATN << "/bye"
       << COL::APP << " , to exit" << COL::DEF << endl;
  std::string prompt;
  cout << COL::USR << "PROMPT: ";
  while (getline(cin, prompt)) {

    if (prompt == "/help") {
      show_chat_help();
    } else if (prompt == "/new") {
      cout << COL::APP
           << "Starting a new conversation, clearing previous chat context"
           << COL::DEF << endl;
      history.clear();
    } else if (prompt == "/debug") {
      opt.debug = !opt.debug;
      cout << COL::ATN << "Toggled Debug, debug is now " << opt.debug
           << COL::DEF << endl;
    } else if (prompt == "/bye") {
      cout << COL::ATN << "Exiting Chat..." << COL::DEF << endl;
      break;
    } else {
      try {
        ochat::SendRequestToAi(prompt, history);

      } catch (const std::runtime_error &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        ret = 1;
        break;
      }
    }
    cout << COL::USR << "PROMPT: ";
  }

  return ret;
}
