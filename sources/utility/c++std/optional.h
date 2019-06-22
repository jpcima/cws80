#if __cplusplus >= 201703L && ((defined(__cpp_lib_optional) || __has_include(<optional>)))
#include <optional>
namespace cxx {
template <class T> using optional = std::optional<T>;
using std::bad_optional_access;
}  // namespace cxx
#elif 1
#include <nonstd/optional.hpp>
namespace cxx {
template <class T> using optional = nonstd::optional<T>;
using nonstd::bad_optional_access;
}  // namespace cxx
#else
#include <boost/optional.hpp>
namespace cxx {
template <class T> using optional = boost::optional<T>;
using boost::bad_optional_access;
}  // namespace cxx
#endif
