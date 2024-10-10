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

// Define colors for each context
namespace COLOR {
constexpr const char *ATTN = USE_COLORS ? "\033[31m" : "";    // red
constexpr const char *AI = USE_COLORS ? "\033[32m" : "";      // green
constexpr const char *WARN = USE_COLORS ? "\033[33m" : "";    // yellow
constexpr const char *APP = USE_COLORS ? "\033[34m" : "";     // blue
constexpr const char *USER = USE_COLORS ? "\033[0m" : "";     // default
constexpr const char *DEFAULT = USE_COLORS ? "\033[0m" : "";  // default
};  // namespace COLOR

#endif /* APP_CONFIG_H */
