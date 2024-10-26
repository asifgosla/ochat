# OChat

## Overview

Ochat is a C++ application to chat with an LLM using the Ollama APIs. Admittedly, this app is a bit contrived, as Ollama already has chat functionality. However, the main purpose of this app is to demonstrate the following:

- Calling an Ollama REST API from C++ and receiving a chunked response.
- Processing JSON response.
- Using the Bazel build system.
- Unit tests with gTest/gMock, and Code Coverage.

## Technologies

- C++, C++17, C++20
- REST API client (using boost::asio)
  - includes handling chunked response from Ollama.
- JSON (using boost::json)
- Bazel build system
- Unit Test, GoogleTest (gTest & gMock).
- Ollama

## Dependencies

- bazel is required to build this project.
  - refer to this [link](https://github.com/asifgosla/gtest-example?tab=readme-ov-file#install-bazel) for installation instructions.
- gTest, gMock - these are automatically downloaded by bazel when building the project.
- boost::asio, boost::json - thes are automatically be downloaded by bazel when building the project.
- ollama - this can be downloaded from https://ollama.ai/download and needs to be installed locally.

## Tests and Code Coverage

### Running Unit Tests Only

In order to run all the unit tests `bazel test //... --test_output=all`

### Running Unit Tests with Code Coverage

follow the following steps to run the unit tests with code coverage and generate an html code coverage report:

1. Ensure lcov is installed on your system.

- `sudo apt-get update && sudo apt-get install lcov`
- create a coverage subdirector under `ochat` directory.

2. Run the tests and collect coverage

- For combined code coverage data across all tests, run the script `./runtests_with_coverage.sh` from the `ochat` directory. This will generate a combined coverage report under `ochat/coverage` directory.

- Alternatively, to run a single test with coverage, issue the following commands from the `ochat` directory:

  - `bazel test --collect_code_coverage --test_output=all //:ochat_format_test`
  - `cp $(bazel info bazel-testlogs)/ochat_format_test/coverage.dat .`
  - `genhtml ./coverage.dat -o coverage`

  ##### Note that I am running in a vscode dev container and if try and use the coverage.dat file from the default output location I get The error message _genhtml: ERROR: not a plain file: /home/vscode/.cache/bazel/..." indicates that the genhtml tool is unable to process the specified file because it is not a regular file._ This could be due to the /home/vscode not being a real directory but something created by the devcontainer. To workaround this issue I copied the file to my project directory in the above steps.

- Open the file `.\coverage/index.html` in your browser to examine the code coverage report.

### Mocking the Boost ASIO interfaces

One of the things that was done to unit test this code was to implement mocks for interactions with the boost::asio socket read and write functions.
The approach that was taken in this code was to leverage template specialization in the unit test build to allow intercepting the boost template functions
that were needed for testing and having these functions forward the calls to the MockAsio class members. Note that using weak linkage does not work with template classes. Anoter option would have been to wrap the boost asio api calls in a class and using that from the production code, but the current approach was chosen to have direct access to the boost apis in the production code.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
