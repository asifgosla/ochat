/**
 * @file app_config.h
 * @brief Header file that contains application configuration switches
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// switches that can be passed in at compile time
#define ENABLE_DEBUG_LOG 0
#define USE_COLORS 1
#define OLLAMA_SERVER_ADDR "host.docker.internal"
#define OLLAMA_SERVER_PORT 11434
#define OLLAMA_ENDPOINT "/api/chat"
#define OLLAMA_MODEL "llama3.2:1b"
#define OLLAMA_STREAM_RESP true

// Define colors for each context
namespace COL {
constexpr const char *ATN = USE_COLORS ? "\033[31m" : ""; // red
constexpr const char *AI = USE_COLORS ? "\033[32m" : "";  // green
constexpr const char *WRN = USE_COLORS ? "\033[33m" : ""; // yellow
constexpr const char *APP = USE_COLORS ? "\033[34m" : ""; // blue
constexpr const char *USR = USE_COLORS ? "\033[0m" : "";  // default
constexpr const char *DEF = USE_COLORS ? "\033[0m" : "";  // default
}; // namespace COL

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#endif /* APP_CONFIG_H */
