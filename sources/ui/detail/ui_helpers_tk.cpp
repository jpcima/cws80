#include "ui/detail/ui_helpers_tk.h"
#include "utility/debug.h"
#include <tcl.h>
#include <tk.h>
#include <memory>

#ifdef USE_TCL_STUBS
#include "dynamic/tcltk.h"
#endif

#if (defined(USE_TCL_STUBS) && !defined(USE_TK_STUBS)) ||   \
    (!defined(USE_TCL_STUBS) && defined(USE_TK_STUBS))
#error invalid Tcl/Tk stubs specification given
#endif

namespace cws80 {

struct Tcl_Interp_Delete { void operator()(Tcl_Interp *x) const noexcept { Tcl_DeleteInterp(x); } };
typedef std::unique_ptr<Tcl_Interp, Tcl_Interp_Delete> Tcl_Interp_u;

std::string TkUI::choose_file(gsl::span<const FileChooserFilter> filters, const std::string &directory, const std::string &title, FileChooserMode mode)
{
    Tcl_Interp_u interp(create_tk_interp());
    if (!interp)
        return {};

    std::ostringstream script;
    script <<
        ((mode == FileChooserMode::Save) ? "tk_getSaveFile" : "tk_getOpenFile") <<
        " -title " << quote(title) <<
        " -initialdir " << quote(directory);
    if (!filters.empty()) {
        script << " -filetypes {";
        for (const FileChooserFilter &filter : filters) {
            script << " { " << quote(filter.name) << " {";
            for (const char *pattern : filter.patterns) {
                if (pattern[0] == '*') {
                    if (pattern[1] == '.')
                        script << " " << quote(pattern + 1);
                    else if (pattern[1] == '\0')
                        script << " " << "{*}";
                }
            }
            script << " } }";
        }
        script << " }";
    }

    if (Tcl_Eval(interp.get(), script.str().c_str()) == TCL_ERROR) {
#ifndef NDEBUG
        const char *result = Tcl_GetStringResult(interp.get());
        debug("{}", result ? result : "Tcl_Eval failed");
#endif
        return {};
    }

    const char *result = Tcl_GetStringResult(interp.get());
    if (!result) {
        debug("Tcl_GetStringResult failed");
        return {};
    }

    return result;
}

cxx::optional<std::string> TkUI::edit_line(const std::string &title, const std::string &initial_value)
{
    Tcl_Interp_u interp(create_tk_interp());
    if (!interp)
        return {};

    const char tk_getstring_module[] = {
        #include "ui/data/tk_getString.dat.h"
        '\0'
    };

    if (Tcl_Eval(interp.get(), tk_getstring_module) == TCL_ERROR) {
#ifndef NDEBUG
        const char *result = Tcl_GetStringResult(interp.get());
        debug("{}", result ? result : "Tcl_Eval failed");
#endif
        return {};
    }

    std::ostringstream script;
    script <<
        "set text " << quote(initial_value) << "\n"
        "::getstring::tk_getString .dialog result {Please enter:}"
        " -title " << quote(title) <<
        " -entryoptions {-textvariable text}" << "\n"
        "return $result";

    if (Tcl_Eval(interp.get(), script.str().c_str()) == TCL_ERROR) {
#ifndef NDEBUG
        const char *result = Tcl_GetStringResult(interp.get());
        debug("{}", result ? result : "Tcl_Eval failed");
#endif
        return {};
    }

    const char *result = Tcl_GetStringResult(interp.get());
    if (!result) {
        debug("Tcl_GetStringResult failed");
        return {};
    }

    return std::string(result);
}

uint TkUI::select_by_menu(gsl::span<const std::string> choices, uint initial_selection)
{
    Tcl_Interp_u interp(create_tk_interp());
    if (!interp)
        return ~0u;

    std::ostringstream script;
    script <<
        "set state {}" "\n"
        "set menu [menu .menu]" "\n"
        "bind .menu <Unmap> {if {$state == {}} {set state -1}}" "\n";
    for (size_t i = 0, n = choices.size(); i < n; ++i) {
        script <<
            "$menu add command -label " << quote(choices[i]) << " -command {set state " << i << "}" "\n";
    }
    script <<
        "tk_popup .menu [winfo pointerx .] [winfo pointery .]";
    if (initial_selection != ~0u)
        script << ' ' << initial_selection;
    script << "\n"
        "tkwait variable state" "\n"
        "return $state";

    if (Tcl_Eval(interp.get(), script.str().c_str()) == TCL_ERROR) {
#ifndef NDEBUG
        const char *result = Tcl_GetStringResult(interp.get());
        debug("{}", result ? result : "Tcl_Eval failed");
#endif
        return ~0u;
    }

    const char *result = Tcl_GetStringResult(interp.get());
    if (!result) {
        debug("Tcl_GetStringResult failed");
        return ~0u;
    }

    uint selection = 0;
    if (Tcl_GetInt(interp.get(), result, (int *)&selection) == TCL_ERROR) {
        debug("Tcl_GetInt failed");
        return ~0u;
    }

    return selection;
}

#ifdef USE_TCL_STUBS
static constexpr char tcltk_version_desired[] = "8.4";
#undef Tcl_CreateInterp
#undef Tcl_Init
#undef Tk_Init
#define Tcl_CreateInterp tcltk.dynamic_Tcl_CreateInterp
#define Tcl_Init tcltk.dynamic_Tcl_Init
#define Tk_Init tcltk.dynamic_Tk_Init
#endif

Tcl_Interp *TkUI::create_tk_interp()
{
#ifdef USE_TCL_STUBS
    static const Dynamic_TclTk tcltk(tcltk_version_desired);
    if (!tcltk)
        return nullptr;
#endif

    Tcl_Interp_u interp(Tcl_CreateInterp());
    if (!interp) {
        debug("Tcl_CreateInterp failed");
        return nullptr;
    }

    if (Tcl_Init(interp.get()) == TCL_ERROR) {
        debug("Tcl_Init failed");
        return nullptr;
    }
    if (Tk_Init(interp.get()) == TCL_ERROR) {
        debug("Tk_Init failed");
        return nullptr;
    }

#ifdef USE_TCL_STUBS
    if (!Tcl_InitStubs(interp.get(), tcltk_version_desired, 0)) {
        debug("Tcl_InitStubs failed");
        return nullptr;
    }
    if (!Tk_InitStubs(interp.get(), tcltk_version_desired, 0)) {
        debug("Tk_InitStubs failed");
        return nullptr;
    }
#endif

    if (Tcl_Eval(interp.get(), "wm withdraw .") == TCL_ERROR) {
#ifndef NDEBUG
        const char *result = Tcl_GetStringResult(interp.get());
        debug("{}", result ? result : "Tcl_Eval failed");
#endif
        return nullptr;
    }

    return interp.release();
}

std::string TkUI::quote(gsl::cstring_span str)
{
    std::string buf;
    size_t count = 2;
    for (char ch : str)
        count += (ch == '{' || ch == '}') ? 2 : 1;
    buf.reserve(count);
    buf.push_back('{');
    for (char ch : str) {
        if (ch == '{' || ch == '}') buf.push_back('\\');
        buf.push_back(ch);
    }
    buf.push_back('}');
    return buf;
}

}  // namespace cws80
