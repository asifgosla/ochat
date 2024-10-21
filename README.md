# OChat

## Overview

A C++ application to chat with an LLM using Ollama.<br>
This command line application demonstrates calling an Ollama REST API, receiving the chunked post response, and processing the JSON response.
Additionally it demonstrates the use of C++ and the Bazel build system.

## Technologies

- C++, C++17, C++20
- REST API client (using boost::asio)
  - includes handling chunked response from Ollama.
- JSON (using boost::json)
- Bazel
- Ollama

## Dependencies

- bazel is required to build this project.
  - refer to this [link](https://github.com/asifgosla/gtest-example?tab=readme-ov-file#install-bazel) for installation instructions.
- boost::asio, boost::json - this will automatically be installed by bazel when building the project.
- ollama - this can be downloaded from https://ollama.ai/download and needs to be installed locally.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
