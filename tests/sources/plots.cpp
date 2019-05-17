#include "plots.h"

std::string gp_escape(cxx::string_view str)
{
    size_t len = str.size();
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        char buf[8];
        uint nchars = sprintf(buf, "\\%.3o", (u8)str[i]);
        result.append(buf, nchars);
    }
    return result;
}
