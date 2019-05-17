#pragma once
#include "utility/types.h"
#include "utility/c++std/optional.h"
#include <gsl/gsl>
#include <vector>
#include <string>

namespace cws80 {

enum class FileChooserMode {
    Open,
    Save,
};

struct FileChooserFilter {
    const char *name;
    std::vector<const char *> patterns;
};

class NativeUI {
public:
    virtual ~NativeUI() {}

    static NativeUI *create();

    virtual std::string choose_file(gsl::span<const FileChooserFilter> filters, const std::string &directory, const std::string &title, FileChooserMode mode) = 0;
    virtual cxx::optional<std::string> edit_line(const std::string &title, const std::string &initial_value) = 0;
    virtual uint select_by_menu(gsl::span<const std::string> choices, uint initial_selection) = 0;
};

}  // namespace cws80
