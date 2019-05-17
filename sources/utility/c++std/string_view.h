#if __cplusplus >= 201703L && ((defined(__cpp_lib_string_view) || __has_include(<string_view>)))
#include <string_view>
namespace cxx {
typedef std::string_view string_view;
typedef std::wstring_view wstring_view;
typedef std::u16string_view u16string_view;
typedef std::u32string_view u32string_view;
template <class C, class T>
using basic_string_view = std::basic_string_view<C, T>;
}  // namespace cxx
#else
#include <boost/utility/string_view.hpp>
namespace cxx {
typedef boost::string_view string_view;
typedef boost::wstring_view wstring_view;
typedef boost::u16string_view u16string_view;
typedef boost::u32string_view u32string_view;
template <class C, class T>
using basic_string_view = boost::basic_string_view<C, T>;
}  // namespace cxx
#endif
