#include "utility/load_library.h"
struct Tcl_Interp;

struct Dynamic_TclTk {
    Dl_Handle_U handle_Tcl;
    Dl_Handle_U handle_Tk;

    Tcl_Interp *(*dynamic_Tcl_CreateInterp)() = nullptr;
    int (*dynamic_Tcl_Init)(Tcl_Interp *) = nullptr;
    int (*dynamic_Tk_Init)(Tcl_Interp *) = nullptr;

    Dynamic_TclTk() noexcept {}
    explicit Dynamic_TclTk(const char *version);

    explicit operator bool() const noexcept
        { return handle_Tcl != nullptr; }

private:
    bool load_from_handles(Dl_Handle_U htcl, Dl_Handle_U htk);
};
