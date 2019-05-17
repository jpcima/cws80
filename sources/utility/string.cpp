#include "utility/string.h"

std::string capitalize(cxx::string_view text)
{
    std::string outp;
    char front = text.length() ? text.front() : '\0';
    outp.reserve(text.size());
    if (front >= 'a' && front <= 'z') {
        outp.push_back(front - 'a' + 'A');
        outp.append(text.begin() + 1, text.end());
    }
    else {
        outp.assign(text.begin(), text.end());
    }
    return outp;
}
