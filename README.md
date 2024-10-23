# OChat

## Overview

Ochat is a C++ application to chat with an LLM using the Ollama APIs. Admittedly, this app is a bit contrived, as Ollama already has chat functionality. However, the main purpose of this app is to demonstrate the following:

- Calling an Ollama REST API from C++ and receiving a chunked response.
- Processing JSON response
- Using the Bazel build system, Unit tests with Google Test, and Code Coverage.

## Technologies

- C++, C++17, C++20
- REST API client (using boost::asio)
  - includes handling chunked response from Ollama.
- JSON (using boost::json)
- Bazel
- Ollama
- Unit Test, Google Test

## Dependencies

- bazel is required to build this project.
  - refer to this [link](https://github.com/asifgosla/gtest-example?tab=readme-ov-file#install-bazel) for installation instructions.
- boost::asio, boost::json - this will automatically be installed by bazel when building the project.
- ollama - this can be downloaded from https://ollama.ai/download and needs to be installed locally.

## Tests and Code Coverage

### Running Unit Tests Only

In order to run the unit tests `bazel test ochat_test --test_output=all`

### Running Unit Tests with Code Coverage

follow the following steps to run the unit tests with code coverage and generate an html code coverage report:

1. Ensure lcov is installed on your system.

- `sudo apt-get update && sudo apt-get install lcov`
- create a coverage subdirector under `ochat` directory.

2. Run the tests and collect coverage data by running the following command in the `ochat` directory:

- `bazel test --collect_code_coverage --test_output=all //:ochat_test`
- `cp $(bazel info bazel-testlogs)/ochat_test/coverage.dat .`
- `genhtml ./coverage.dat -o coverage`
  - Note that I am running in a vscode dev container and if try and use the coverage.dat file from the default output location I get The error message _genhtml: ERROR: not a plain file: /home/vscode/.cache/bazel/..." indicates that the genhtml tool is unable to process the specified file because it is not a regular file._ This could be due to the /home/vscode not being a real directory but something created by the devcontainer. To workaround this issue I copied the file to my project directory in the above steps.
- Open the file `.\coverage/index.html` in your browser to examine the code coverage report.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
