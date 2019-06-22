#include "tcltk.h"
#include "utility/debug.h"
#include <tcl.h>
#include <tk.h>
#include <fmt/format.h>
#include <gsl/gsl>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

#ifdef __gnu_linux__
#include <link.h>
#include <dirent.h>
#endif

static bool tcltk_version_satisfying(
    unsigned vermaj, unsigned vermin, unsigned refmaj, unsigned refmin)
{
    return vermaj == refmaj && vermin >= refmin;
}

#ifdef __gnu_linux__
static std::vector<std::string> find_process_library_dirs()
{
    std::vector<std::string> dirs;
    dl_iterate_phdr(
        +[](struct dl_phdr_info *info, size_t, void *data) -> int {
             std::vector<std::string> &dirs = *(std::vector<std::string> *)data;
             const char *path = info->dlpi_name;
             const char *separator;
             if (path[0] == '/' && (separator = strrchr(path, '/'))) {
                 std::string dir(path, separator + 1);
                 if (std::find(dirs.begin(), dirs.end(), dir) == dirs.end())
                     dirs.push_back(std::move(dir));
             }
             return 0;
         }, &dirs);
    return dirs;
}
#endif

Dynamic_TclTk::Dynamic_TclTk(const char *version)
{
    unsigned refmaj;
    unsigned refmin;
    if (sscanf(version, "%u.%u", &refmaj, &refmin) != 2)
        return;

#ifdef __gnu_linux__
    // collect known library search paths
    const std::vector<std::string> dirs = find_process_library_dirs();
#ifndef NDEBUG
    for (const std::string &dir : dirs)
        debug("Process library path: {}", dir);
#endif
    // search file libtclN.M.so
    bool found = false;
    for (size_t i = 0, n = dirs.size(); i < n && !found; ++i) {
        const std::string &dir_path = dirs[i];
        debug("Search Tcl/Tk {}+ in directory {}", version, dir_path);

        DIR *dir = opendir(dir_path.c_str());
        if (!dir) continue;
        auto dir_cleanup = gsl::finally([dir] { closedir(dir); });

        while (dirent *ent = readdir(dir)) {
            const char *name = ent->d_name;
            unsigned vermaj, vermin;
            unsigned scan_num;
            if (sscanf(name, "libtcl%u.%u.so%n", &vermaj, &vermin, &scan_num) == 2 &&
                scan_num == strlen(name) && tcltk_version_satisfying(vermaj, vermin, refmaj, refmin))
            {
                // attempt load of tcl/tk library pair
                std::string tcl_path = fmt::format("{}{}", dir_path, name);
                std::string tk_path = fmt::format("{}libtk{}.{}.so", dir_path, vermaj, vermin);
                Dl_Handle_U htcl(Dl_open(tcl_path.c_str()));
                if (htcl) {
                    Dl_Handle_U htk(Dl_open(tk_path.c_str()));
                    found = load_from_handles(std::move(htcl), std::move(htk));
                }
                debug("Tcl/Tk dynamic load: {} version {}.{}", found ? "success" : "failure", vermaj, vermin);
            }
        }
    }
    if (!found)
        debug("Cannot find Tcl/Tk");
#else
    #pragma message("library search not implemented for this platform")
    debug("Unimplemented Tcl/Tk search on this platform");
#endif
}

bool Dynamic_TclTk::load_from_handles(Dl_Handle_U htcl, Dl_Handle_U htk)
{
    if (!htcl || !htk ||
        !(dynamic_Tcl_CreateInterp = (Tcl_Interp *(*)())dlsym(htcl.get(), "Tcl_CreateInterp")) ||
        !(dynamic_Tcl_Init = (int (*)(Tcl_Interp *))dlsym(htcl.get(), "Tcl_Init")) ||
        !(dynamic_Tk_Init = (int (*)(Tcl_Interp *))dlsym(htk.get(), "Tk_Init")))
    {
        return false;
    }
    handle_Tcl = std::move(htcl);
    handle_Tk = std::move(htk);
    return true;
}
