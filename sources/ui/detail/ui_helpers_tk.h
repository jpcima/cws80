#include "ui/detail/ui_helpers_native.h"
#include <gsl/gsl>
struct Tcl_Interp;

namespace cws80 {

class TkUI : public NativeUI {
public:
    std::string choose_file(gsl::span<const FileChooserFilter> filters, const std::string &directory, const std::string &title, FileChooserMode mode) override;
    cxx::optional<std::string> edit_line(const std::string &title, const std::string &initial_value) override;
    uint select_by_menu(gsl::span<const std::string> choices, uint initial_selection) override;

private:
    static Tcl_Interp *create_tk_interp();
    static std::string quote(gsl::cstring_span str);
};

}  // namespace cws80
