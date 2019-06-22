#pragma once

#if defined(__GNUC__)
#  define ForceInline inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#  define ForceInline __forceinline
#else
#  define ForceInline inline
#endif
