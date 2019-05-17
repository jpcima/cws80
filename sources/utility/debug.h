#pragma once
#include <fmt/format.h>
#include <boost/core/demangle.hpp>
#include <cstdio>
#include <cstddef>

#define DEBUG_INCLUDE_SOURCE_LOCATION 1
#define DEBUG_INCLUDE_THREAD_ID 1
#define DEBUG_SOURCE_LOCATION_FORMAT "{:>30}:{:<5}  "
#define DEBUG_FILE_NAME ((const char *)::debug_detail::ct_basename(__FILE__).s)

#ifdef NDEBUG

#define debug(...)
#define debug_exception(...)

#else

#define debug(f, ...)                           \
  ::debug_detail::print(                        \
      "D: "                                     \
       DEBUG_SOURCE_LOCATION_FORMAT f,          \
       DEBUG_FILE_NAME, __LINE__,               \
       ##__VA_ARGS__)
#define debug_exception(ex)                                    \
  do {                                                         \
    const std::exception &ex__##__LINE__ = (ex);               \
    const char *msg__##__LINE__ = ex__##__LINE__.what();       \
    msg__##__LINE__ = msg__##__LINE__ ?                        \
                      msg__##__LINE__ : "<no message>";        \
    ::debug_detail::print(                                     \
        "D: "                                                  \
        DEBUG_SOURCE_LOCATION_FORMAT "exception {}: {}",       \
        DEBUG_FILE_NAME, __LINE__,                             \
        boost::core::demangle(typeid(ex__##__LINE__).name()),  \
        msg__##__LINE__);                                      \
  } while (0)

namespace debug_detail {

template <class... A> void print(const char *f, A &&... args);
template <size_t N> constexpr auto ct_basename(const char (&c)[N]);

}  // namespace debug_detail

//------------------------------------------------------------------------------
namespace debug_detail {

template <class... A>
inline void print(const char *f, A &&... args)
{
#ifndef _WIN32
    FILE *out = stderr;
    bool isterm = isatty(fileno(out)) != 0;
    if (isterm) fputs("\033[36m", stderr);
    fmt::print(out, f, std::forward<A>(args)...);
    if (isterm) fputs("\033[0m", stderr);
    fputc('\n', out);
#else
    OutputDebugStringA(fmt::format(f, std::forward<A>(args)...).c_str());
#endif
}

template <size_t N>
inline constexpr auto ct_basename(const char (&c)[N])
{
  constexpr size_t n = N - 1;
#if defined(_WIN32)
  constexpr char os_sep = '\\';
#else
  constexpr char os_sep = 0;
#endif
  size_t i = n;
  for (; i > 0 && c[i-1] != '/' && (!os_sep || c[i-1] != os_sep); --i);
  size_t nb = (i > 0) ? (n - i) : n;
  struct ct_string_buf { char s[N]; } b {};
  for (size_t i = 0; i < nb; ++i)
    b.s[i] = c[n - nb + i];
  return b;
}

}  // namespace debug_detail

#endif
