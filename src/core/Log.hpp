#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>

namespace velora::log {

enum Level { Debug, Info, Warn, Error };

inline const char* levelTag(Level l) {
  switch (l) {
    case Debug: return "DEBUG";
    case Info:  return "INFO";
    case Warn:  return "WARN";
    default:    return "ERROR";
  }
}

inline const char* levelColor(Level l) {
  switch (l) {
    case Debug: return "\033[90m";
    case Info:  return "\033[36m";
    case Warn:  return "\033[33m";
    default:    return "\033[31m";
  }
}

template <typename... Args>
inline void message(Level lvl, const char* fmt, Args... args) {
  std::fprintf(stderr, "%s[%s]\033[0m ", levelColor(lvl), levelTag(lvl));
  std::fprintf(stderr, fmt, args...);
  std::fprintf(stderr, "\n");
}

template <typename... Args> inline void debug(const char* f, Args... a) { message(Debug, f, a...); }
template <typename... Args> inline void info(const char* f, Args... a)  { message(Info,  f, a...); }
template <typename... Args> inline void warn(const char* f, Args... a)  { message(Warn,  f, a...); }
template <typename... Args> inline void error(const char* f, Args... a) { message(Error, f, a...); }

[[noreturn]] inline void fatal(const std::string& msg) {
  message(Error, "fatal: %s", msg.c_str());
  std::exit(1);
}

} // namespace velora::log
